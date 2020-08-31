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

#ifndef __COM_EXECUTIONMANAGER_H__
#define __COM_EXECUTIONMANAGER_H__

#include "COM_Rect.h"
#include "COM_defines.h"
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif

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
  int m_n_optimized_ops;
  int m_n_exec_operations;
  int m_n_exec_subworks;
  int m_n_subworks;
  std::mutex mutex;

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

  void reportOperationOptimized(NodeOperation *op);
  // must be called by BufferManager when the full operation write is completed
  void reportOperationCompleted(NodeOperation *op);
  // must be called by WorkPackage on completed
  void reportSubworkCompleted();
  static void deviceWaitQueueToFinish();

 private:
  // returns null if operation has no viewer border
  const rcti *getOpViewerBorder(NodeOperation *op);
  void updateProgress(int n_exec_subworks = 0, int n_total_subworks = 0);

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:ExecutionManager");
#endif
};

#endif
