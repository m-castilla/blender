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

#include "BLI_string.h"
#include "BLT_translation.h"
#include <algorithm>

#include "COM_ComputeDevice.h"
#include "COM_ExecutionGroup.h"
#include "COM_ExecutionManager.h"
#include "COM_ExecutionSystem.h"
#include "COM_GlobalManager.h"
#include "COM_RectUtil.h"
#include "COM_WorkScheduler.h"

ExecutionManager::ExecutionManager(const CompositorContext &context,
                                   std::vector<ExecutionGroup *> &exec_groups)
    : m_context(context),
      m_exec_groups(exec_groups),
      m_op_mode(OperationMode::Optimize),
      m_n_optimized_ops(0),
      m_n_exec_operations(0),
      m_n_exec_subworks(0),
      m_n_subworks(0),
      mutex()
{
}

bool ExecutionManager::isBreaked() const
{
  return m_context.isBreaked();
}

void ExecutionManager::setOperationMode(OperationMode mode)
{
  m_op_mode = mode;
}
OperationMode ExecutionManager::getOperationMode() const
{
  return m_op_mode;
}
bool ExecutionManager::canExecPixels() const
{
  return !m_context.isBreaked() && m_op_mode == OperationMode::Exec;
}

void ExecutionManager::execWriteJob(
    NodeOperation *op,
    TmpRectBuilder &write_rect_builder,
    std::function<void(PixelsRect &, const WriteRectContext &)> &cpu_write_func,
    std::function<void(PixelsRect &)> after_write_func,
    std::string compute_kernel,
    std::function<void(ComputeKernel *)> add_kernel_args_func)
{
  if (!isBreaked()) {
    // in case of output with viewer border set operation rect to viewer border
    int xmin = 0;
    int ymin = 0;
    int xmax = op->getWidth();
    int ymax = op->getHeight();
    const rcti *viewer_border = getOpViewerBorder(op);
    if (viewer_border != nullptr) {
      xmin = std::max(viewer_border->xmin, xmin);
      ymin = std::max(viewer_border->ymin, ymin);
      xmax = std::min(viewer_border->xmax, xmax);
      ymax = std::min(viewer_border->ymax, ymax);
    }

    rcti full_op_rect = {xmin, xmax, ymin, ymax};
    std::shared_ptr<PixelsRect> full_write_rect = write_rect_builder(full_op_rect);

    std::vector<WorkPackage *> works;
    bool is_computed = op->isComputed(*this);
    if (op->getWriteType() == WriteType::SINGLE_THREAD) {
      m_n_subworks = 1;
      m_n_exec_subworks = 0;
      WorkPackage *work = new WorkPackage(*this, full_write_rect, cpu_write_func);
      works.push_back(work);
    }
    else if (is_computed) {
      GlobalMan->ComputeMan->getSelectedDevice()->queueJob(
          *full_write_rect, compute_kernel, add_kernel_args_func);
    }
    else {
      int width = xmax - xmin;
      int height = ymax - ymin;

      int n_total_works = m_context.getNCpuWorkThreads() * 8;
      n_total_works = std::min(n_total_works, height);
      std::vector<rcti> splits = RectUtil::splitImgRectInEqualRects(n_total_works, width, height);
      m_n_subworks = splits.size();
      m_n_exec_subworks = 0;

      for (auto &rect : splits) {
        if (xmin > 0) {
          rect.xmin += xmin;
          rect.xmax += xmin;
        }
        if (ymin > 0) {
          rect.ymin += ymin;
          rect.ymax += ymin;
        }

        std::shared_ptr<PixelsRect> w_rect = write_rect_builder(rect);
        WorkPackage *work = new WorkPackage(*this, w_rect, cpu_write_func);
        works.push_back(work);
      }
    }

    int n_passes = op->getNPasses();
    int current_pass = 0;
    while (current_pass < n_passes) {
      for (auto work : works) {
        work->reset();
        WriteRectContext ctx;
        ctx.n_passes = n_passes;
        ctx.n_rects = works.size();
        ctx.current_pass = current_pass;
        work->setWriteContext(ctx);
        WorkScheduler::schedule(work);
      }

      bool finished = false;
      while (!finished) {
        WorkScheduler::finish();
        finished = true;
        for (WorkPackage *work : works) {
          if (!work->hasFinished()) {
            finished = false;
            break;
          }
        }
      }
      current_pass++;
    }

    for (WorkPackage *work : works) {
      delete work;
    }

    if (after_write_func && !isBreaked()) {
      if (is_computed) {
        deviceWaitQueueToFinish();
      }
      after_write_func(*full_write_rect);
    }
  }
}

void ExecutionManager::updateProgress(int n_exec_subworks, int n_total_subworks)
{
  auto tree = m_context.getbNodeTree();
  if (tree) {
    float progress = (float)m_n_exec_operations / (float)m_n_optimized_ops;
    if (n_exec_subworks > 0) {
      BLI_assert(n_exec_subworks >= 0 && n_exec_subworks <= n_total_subworks);
      float subwork_range = ((m_n_exec_operations + 1.0f) / (float)m_n_optimized_ops) - progress;
      float added_progress = ((float)n_exec_subworks / (float)n_total_subworks) * subwork_range;
      progress += added_progress;
    }
    tree->progress(tree->prh, progress);

    char buf[128];
    BLI_snprintf(buf,
                 sizeof(buf),
                 TIP_("Compositing | Operation %u-%u"),
                 m_n_exec_operations + 1,
                 m_n_optimized_ops);
    tree->stats_draw(tree->sdh, buf);
  }
}

void ExecutionManager::deviceWaitQueueToFinish()
{
  auto device = GlobalMan->ComputeMan->getSelectedDevice();
  device->waitQueueToFinish();
}

void ExecutionManager::reportSubworkCompleted()
{
  mutex.lock();
  m_n_exec_subworks++;
  updateProgress(m_n_exec_subworks, m_n_subworks);
  mutex.unlock();
}

void ExecutionManager::reportOperationOptimized(NodeOperation *op)
{
  auto buf_type = op->getBufferType();
  if (buf_type != BufferType::NO_BUFFER_NO_WRITE) {
    m_n_optimized_ops++;
  }
}

void ExecutionManager::reportOperationCompleted(NodeOperation *op)
{
  auto buf_type = op->getBufferType();
  if (buf_type != BufferType::NO_BUFFER_NO_WRITE) {
    m_n_exec_operations++;
    updateProgress();
  }
}

const rcti *ExecutionManager::getOpViewerBorder(NodeOperation *op)
{
  if (op->isOutputOperation(m_context.isRendering())) {
    for (auto group : m_exec_groups) {
      if (group->getOutputOperation() == op) {
        if (group->hasOutputViewerBorder()) {
          return &group->getOutputViewerBorder();
        }
        else {
          return nullptr;
        }
      }
    }
  }
  return nullptr;
}
