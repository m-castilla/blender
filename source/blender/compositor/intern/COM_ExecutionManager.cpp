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

#include "COM_ExecutionManager.h"
#include "COM_ComputeDevice.h"
#include "COM_ExecutionGroup.h"
#include "COM_ExecutionSystem.h"
#include "COM_GlobalManager.h"
#include "COM_RectUtil.h"
#include "COM_WorkScheduler.h"
#include <algorithm>

ExecutionManager::ExecutionManager(const CompositorContext &context,
                                   std::vector<ExecutionGroup *> &exec_groups)
    : m_context(context), m_exec_groups(exec_groups), m_op_mode(OperationMode::Optimize)
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
      WorkPackage *work = new WorkPackage(full_write_rect, cpu_write_func);
      works.push_back(work);
    }
    else if (is_computed) {
      GlobalMan->ComputeMan->getSelectedDevice()->queueJob(
          *full_write_rect, compute_kernel, add_kernel_args_func);
    }
    else {
      int width = xmax - xmin;
      int height = ymax - ymin;

      int n_works = m_context.getNCpuWorkThreads();
      std::vector<rcti> splits = RectUtil::splitImgRectInEqualRects(n_works, width, height);

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
        WorkPackage *work = new WorkPackage(w_rect, cpu_write_func);
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
        // if (isBreaked()) {
        //  WorkScheduler::stop();
        //  break;
        //}
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

void ExecutionManager::deviceWaitQueueToFinish()
{
  auto device = GlobalMan->ComputeMan->getSelectedDevice();
  device->waitQueueToFinish();
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
