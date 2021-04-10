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

#include "COM_ComputeDevice.h"
#include "COM_ComputeManager.h"
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
                                 const char *viewName,
                                 ComputeManager *compute_manager,
                                 int n_cpu_threads)
    : m_context(),
      m_operations(),
      m_n_cpu_work_splits(n_cpu_threads),
      m_n_operations_executed(0),
      m_compute_manager(compute_manager),
      m_gpu_buffer_manager(compute_manager),
      m_cpu_buffer_manager(compute_manager),
      m_output_manager(&m_cpu_buffer_manager, &m_gpu_buffer_manager)
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
  m_n_operations_executed = 0;

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

void ExecutionSystem::execWorkCPU(const rcti &work_rect,
                                  std::function<void(const rcti &split_rect)> &work_func)
{
  /* split work rect vertically to execute multi-threadedly */
  int work_height = BLI_rcti_size_y(&work_rect);
  execWorkCPU(work_height, [&](WorkPackage *work, int split_from, int split_to) {
    work->execute = [=, &work_func, &work_rect]() {
      rcti split_rect;
      BLI_rcti_init(&split_rect,
                    work_rect.xmin,
                    work_rect.xmax,
                    work_rect.ymin + split_from,
                    work_rect.ymin + split_to);
      work_func(split_rect);
    };
  });
}

void ExecutionSystem::execWorkCPU(int work_from,
                                  int work_to,
                                  std::function<void(int split_from, int split_to)> &work_func)
{
  /* split work length in ranges to execute multi-threadedly */
  int work_length = work_to - work_from;
  execWorkCPU(work_length, [=, &work_func](WorkPackage *work, int split_from, int split_to) {
    work->execute = [=, &work_func]() { work_func(work_from + split_from, work_from + split_to); };
  });
}

void ExecutionSystem::execWorkCPU(
    int work_length,
    std::function<void(WorkPackage *, int split_from, int split_to)> config_work_func)
{
  /* Split cpu work */
  int std_split_length = work_length / m_n_cpu_work_splits;
  int last_split_length = std_split_length +
                          (work_length - std_split_length * m_n_cpu_work_splits);
  Vector<WorkPackage *> works;
  for (int i = 0; i < m_n_cpu_work_splits; i++) {
    bool is_last = (i == m_n_cpu_work_splits - 1);
    int split_length = is_last ? last_split_length : std_split_length;
    WorkPackage *work = new WorkPackage();
    int split_from = split_length * i;
    int split_to = split_from + split_length;
    config_work_func(work, split_from, split_to);
    works.append(work);
  }

  /* execute cpu works */
  for (auto work : works) {
    WorkScheduler::schedule(work);
  }
  WorkScheduler::finish();

  /* delete cpu works */
  for (auto work : works) {
    delete work;
  }
}

void ExecutionSystem::execWorkGPU(int work_width,
                                  int work_height,
                                  const StringRef kernel_name,
                                  std::function<void(ComputeKernel *)> add_kernel_args_func)
{
  auto device = m_compute_manager->getSelectedDevice();
  device->enqueueWork(work_width, work_height, kernel_name, add_kernel_args_func);
  device->waitQueueToFinish();
}

void ExecutionSystem::reportOperationEnd()
{
  m_n_operations_executed++;
  updateProgressBar();
}

void ExecutionSystem::updateProgressBar()
{
  auto tree = m_context.getbNodeTree();
  if (tree) {
    int n_operations = m_operations.size();
    float progress = static_cast<float>(m_operations.size()) / m_n_operations_executed;
    tree->progress(tree->prh, progress);

    char buf[128];
    BLI_snprintf(buf,
                 sizeof(buf),
                 TIP_("Compositing | Operation %u-%u"),
                 m_n_operations_executed + 1,
                 n_operations);
    tree->stats_draw(tree->sdh, buf);
  }
}

}  // namespace blender::compositor
