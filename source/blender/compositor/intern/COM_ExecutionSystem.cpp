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

ExecutionSystem::ExecutionSystem(const CompositorContext &context)
    : m_context(context), m_bNodeTree(context.getbNodeTree())
{
  {
    NodeOperationBuilder builder(*this, m_bNodeTree);
    builder.convertToOperations();
  }

  int index;
  int resolution[2];

  rctf *viewer_border = &m_bNodeTree->viewer_border;
  bool use_viewer_border = (m_bNodeTree->flag & NTREE_VIEWER_BORDER) &&
                           viewer_border->xmin < viewer_border->xmax &&
                           viewer_border->ymin < viewer_border->ymax;

  m_bNodeTree->stats_draw(m_bNodeTree->sdh, TIP_("Compositing | Determining resolution"));

  for (index = 0; index < this->m_groups.size(); index++) {
    resolution[0] = 0;
    resolution[1] = 0;
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

  unsigned int index;

  // initialize operations
  for (index = 0; index < this->m_operations.size(); index++) {
    NodeOperation *operation = this->m_operations[index];
    operation->setbNodeTree(m_context.getbNodeTree());
    operation->initExecution();
  }

  for (int index = 0; index < this->m_groups.size(); index++) {
    ExecutionGroup *executionGroup = this->m_groups[index];
    executionGroup->initExecution();
  }

  execGroups(OperationMode::Optimize);
  execGroups(OperationMode::Exec);

  m_bNodeTree->stats_draw(m_bNodeTree->sdh, TIP_("Compositing | De-initializing execution"));
  for (index = 0; index < this->m_operations.size(); index++) {
    NodeOperation *operation = this->m_operations[index];
    operation->deinitExecution();
  }

  for (int index = 0; index < this->m_groups.size(); index++) {
    ExecutionGroup *executionGroup = this->m_groups[index];
    executionGroup->deinitExecution();
  }
}

void ExecutionSystem::execGroups(OperationMode op_mode)
{
  for (int index = 0; index < m_groups.size(); index++) {
    if (!isBreaked()) {
      ExecutionGroup *group = m_groups[index];
      group->setOperationMode(op_mode);
      group->execute();
    }
  }
}
