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

#include "BLI_utildefines.h"
#include "PIL_time.h"

#include "BKE_node.h"

#include "BLT_translation.h"

#include "COM_Converter.h"
#include "COM_Debug.h"
#include "COM_NodeOperation.h"
#include "COM_NodeOperationBuilder.h"
#include "COM_WorkScheduler.h"

#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif

namespace blender::compositor {

ExecutionSystem::ExecutionSystem(RenderData *rd,
                                 Scene *scene,
                                 bNodeTree *editingtree,
                                 bool rendering,
                                 const ColorManagedViewSettings *viewSettings,
                                 const ColorManagedDisplaySettings *displaySettings,
                                 const char *viewName)
{
  this->m_context.setViewName(viewName);
  this->m_context.setScene(scene);
  this->m_context.setbNodeTree(editingtree);
  this->m_context.setPreviewHash(editingtree->previews);
  /* initialize the CompositorContext */
  if (rendering) {
    this->m_context.setQuality((eCompositorQuality)editingtree->render_quality);
  }
  else {
    this->m_context.setQuality((eCompositorQuality)editingtree->edit_quality);
  }
  this->m_context.setRendering(rendering);

  this->m_context.setRenderData(rd);
  this->m_context.setViewSettings(viewSettings);
  this->m_context.setDisplaySettings(displaySettings);

  {
    NodeOperationBuilder builder(&m_context, editingtree);
    builder.convertToOperations(this);
  }

  //  DebugInfo::graphviz(this);
}

ExecutionSystem::~ExecutionSystem()
{
  for (NodeOperation *operation : m_operations) {
    delete operation;
  }
  this->m_operations.clear();
}

void ExecutionSystem::set_operations(const Vector<NodeOperation *> &operations)
{
  m_operations = operations;
}

void ExecutionSystem::execute()
{
  const bNodeTree *editingtree = this->m_context.getbNodeTree();
  const RenderData *rd = this->m_context.getRenderData();

  const rctf *viewer_border = &editingtree->viewer_border;
  bool use_viewer_border = (editingtree->flag & NTREE_VIEWER_BORDER) &&
                           viewer_border->xmin < viewer_border->xmax &&
                           viewer_border->ymin < viewer_border->ymax;
  bool use_render_border = (rd->mode & R_BORDER) && !(rd->mode & R_CROP);
  const rctf *render_border = &rd->border;

  editingtree->stats_draw(editingtree->sdh, TIP_("Compositing | Initializing execution"));

  DebugInfo::execute_started(this);

  WorkScheduler::start(this->m_context);

  /* TODO: execute operations by eCompositorPriority order. Use Viewer/Render border as render rect
   * when enabled otherwise a full frame rect. No need to initialize/deinitialize operations
   * anymore, they'll do it by themselves on start/end of first read. */

  WorkScheduler::finish();
  WorkScheduler::stop();

  editingtree->stats_draw(editingtree->sdh, TIP_("Compositing | De-initializing execution"));
}

}  // namespace blender::compositor
