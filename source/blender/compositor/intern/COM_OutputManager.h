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
 * Copyright 2021, Blender Foundation.
 */

#pragma once

#include "BLI_map.hh"
#include "BLI_span.hh"
#include "BLI_vector.hh"
#include "COM_Buffer.h"
#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif

namespace blender::compositor {

class NodeOperation;
class CPUBufferManager;
class GPUBufferManager;
class OutputManager {
 private:
  typedef struct OutputData {
   public:
    OutputData();
    BaseBufferUniquePtr buffer;
    blender::Vector<rcti> render_rects;
    int registered_reads;
    int received_reads;
  } OutputData;
  blender::Map<NodeOperation *, OutputData> m_outputs;
  CPUBufferManager *m_cpu_man;
  GPUBufferManager *m_gpu_man;

 public:
  OutputManager(CPUBufferManager *cpu_man, GPUBufferManager *gpu_man);
  bool isRenderRegistered(NodeOperation *op, const rcti &render_rect);
  void registerRender(NodeOperation *op, const rcti &render_rect);

  bool hasRegisteredReads(NodeOperation *op);
  void registerRead(NodeOperation *op);

  blender::Span<rcti> getRectsToRender(NodeOperation *op);
  bool isOutputRendered(NodeOperation *op);
  void giveRenderedOutput(NodeOperation *op, CPUBufferUniquePtr<float> buf);
  void giveRenderedOutput(NodeOperation *op, GPUBufferUniquePtr buf);
  const CPUBuffer<float> *getOutputCPU(NodeOperation *op);
  const GPUBuffer *getOutputGPU(NodeOperation *op);

  void reportRead(NodeOperation *read_op);

 private:
  OutputData &getOutputData(NodeOperation *op);

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:OutputManager")
#endif
};

}  // namespace blender::compositor
