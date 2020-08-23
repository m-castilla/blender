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

#ifndef __COM_CHUNKTHREADMANAGER_H__
#define __COM_CHUNKTHREADMANAGER_H__

#include "COM_Rect.h"
#include "COM_defines.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct WriteRectContext;
class ComputeManager;
class BufferManager;
class ExecutionGroup;
struct rcti;
class ComputeKernel;
class NodeOperation;
class CompositorContext;
class ExecutionManager {
 private:
  const CompositorContext &m_context;
  std::vector<ExecutionGroup *> &m_exec_groups;
  OperationMode m_op_mode;

 public:
  ExecutionManager(const CompositorContext &context, std::vector<ExecutionGroup *> &exec_groups);
  bool isBreaked() const;

  void setOperationMode(OperationMode mode);
  OperationMode getOperationMode() const;
  bool canExecPixels() const;

  void execWriteJob(NodeOperation *op,
                    TmpRectBuilder &write_rect_builder,
                    std::function<void(PixelsRect &, const WriteRectContext &)> &cpu_write_func,
                    std::function<void(PixelsRect &)> after_write_func,
                    std::string compute_kernel,
                    std::function<void(ComputeKernel *)> add_kernel_args_func);
  static void deviceWaitQueueToFinish();

 private:
  // returns null if operation has no viewer border
  const rcti *getOpViewerBorder(NodeOperation *op);
};

#endif
