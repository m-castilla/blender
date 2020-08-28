/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Copyright 2011, Blender Foundation.
 */

#include "COM_ExecutionSystem.h"
#include "COM_BufferManager.h"
#include "COM_Converter.h"
#include "COM_Debug.h"
#include "COM_ExecutionGroup.h"
#include "COM_GlobalManager.h"
#include "COM_NodeOperation.h"
#include "COM_NodeOperationBuilder.h"
#include "COM_WorkScheduler.h"
#include "COM_compositor.h"
#include "COM_defines.h"

#include "BLI_utildefines.h"
#include "PIL_time.h"

#include "BKE_node.h"

#include "BLI_string.h"
#include "BLT_translation.h"
#include "MEM_guardedalloc.h"
#include "opencl/COM_OpenCLManager.h"
#include <algorithm>

ExecutionSystem::ExecutionSystem(const CompositorContext &context)
    : m_bNodeTree(context.getbNodeTree()),
      m_context(context),
      m_operations(),
      m_groups(),
      m_readers_reads()
{
  {
    NodeOperationBuilder builder(*this, m_bNodeTree);
    builder.convertToOperations();
  }

  int index;

  rctf *viewer_border = &m_bNodeTree->viewer_border;
  bool use_viewer_border = (m_bNodeTree->flag & NTREE_VIEWER_BORDER) &&
                           viewer_border->xmin < viewer_border->xmax &&
                           viewer_border->ymin < viewer_border->ymax;

  m_bNodeTree->stats_draw(m_bNodeTree->sdh, TIP_("Compositing | Determining resolution"));

  for (index = 0; index < this->m_groups.size(); index++) {
    ExecutionGroup *executionGroup = this->m_groups[index];

    if (context.isRendering()) {
      /* case when cropping to render border happens is handled in
       * compositor output and render layer nodes
       */
      const RenderData *rd = m_context.getRenderData();
      if ((rd->mode & R_BORDER) && !(rd->mode & R_CROP)) {
        executionGroup->setRenderBorder(
            rd->border.xmin, rd->border.xmax, rd->border.ymin, rd->border.ymax);
      }
    }

    if (use_viewer_border) {
      executionGroup->setViewerBorder(
          viewer_border->xmin, viewer_border->xmax, viewer_border->ymin, viewer_border->ymax);
    }
  }
}

ExecutionSystem::~ExecutionSystem()
{
  unsigned int index;
  for (index = 0; index < this->m_operations.size(); index++) {
    NodeOperation *operation = this->m_operations[index];
    delete operation;
  }
  this->m_operations.clear();
  for (index = 0; index < this->m_groups.size(); index++) {
    ExecutionGroup *group = this->m_groups[index];
    delete group;
  }
  this->m_groups.clear();
  m_ops_deps.clear();
}

void ExecutionSystem::set_operations(const Operations &operations, const Groups &groups)
{
  m_operations = operations;
  m_groups = groups;
}

void ExecutionSystem::execute()
{
  m_bNodeTree->stats_draw(m_bNodeTree->sdh, TIP_("Compositing | Initializing execution"));

  DebugInfo::execute_started(this);

  // initialize operations
  for (int index = 0; index < this->m_operations.size(); index++) {
    NodeOperation *operation = this->m_operations[index];
    operation->setbNodeTree(m_context.getbNodeTree());
    operation->initExecution();
  }

  ExecutionManager man(m_context, m_groups);
  man.setOperationMode(OperationMode::Optimize);
  execGroups(man);

  man.setOperationMode(OperationMode::Exec);
  execGroups(man);
  // auto ops_by_deps = getOperationsOrderedByNDepends(man);
  // for (auto dep : ops_by_deps) {
  //  dep.op->getPixels(nullptr, man);
  //}

  m_bNodeTree->stats_draw(m_bNodeTree->sdh, TIP_("Compositing | De-initializing execution"));
  for (int index = 0; index < this->m_operations.size(); index++) {
    NodeOperation *operation = this->m_operations[index];
    operation->deinitExecution();
  }
}

void ExecutionSystem::execGroups(ExecutionManager &man)
{
  for (int index = 0; index < m_groups.size(); index++) {
    if (!isBreaked()) {
      ExecutionGroup *group = m_groups[index];
      group->execute(man);
    }
  }
}

std::vector<ExecutionSystem::OpDeps> ExecutionSystem::getOperationsOrderedByNDepends(
    ExecutionManager &man)
{
  BLI_assert(m_ops_deps.empty());
  m_readers_reads = GlobalMan->BufferMan->getReadersReads(man);
  for (auto op : m_operations) {
    getOperationNDepends(op);
  }

  std::vector<OpDeps> deps;
  for (auto &entry : m_ops_deps) {
    deps.push_back(entry.second);
  }
  std::sort(deps.begin(), deps.end(), OpDepsSorter());
  return deps;
}

int ExecutionSystem::getOperationNDepends(NodeOperation *op)
{
  OpKey op_key = op->getKey();
  auto deps_found = m_ops_deps.find(op_key);
  if (deps_found != m_ops_deps.end()) {
    return deps_found->second.n_deps;
  }
  else {
    auto reader_found = m_readers_reads->find(op_key);
    OpDeps deps = {op, 0};
    deps.op = op;
    if (reader_found != m_readers_reads->end()) {
      auto reads = reader_found->second;
      deps.n_deps += reads.size();
      for (auto read : reads) {
        BLI_assert(read->reads->readed_op != op);
        deps.n_deps += getOperationNDepends(read->reads->readed_op);
      }
    }
    m_ops_deps.insert({op_key, deps});
    return deps.n_deps;
  }
}
