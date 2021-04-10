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

#include "COM_OutputManager.h"
#include "BLI_rect.h"
#include "COM_BufferManager.h"
#include "COM_NodeOperation.h"

namespace blender::compositor {

OutputManager::OutputManager(CPUBufferManager *cpu_man, GPUBufferManager *gpu_man)
    : m_outputs(), m_cpu_man(cpu_man), m_gpu_man(gpu_man)
{
}
OutputManager::OutputData::OutputData()
    : buffer(nullptr), render_rects(), registered_reads(0), received_reads(0)
{
}

OutputManager::OutputData &OutputManager::getOutputData(NodeOperation *op)
{
  return m_outputs.lookup_or_add_cb(op, []() { return OutputData(); });
}

bool OutputManager::isRenderRegistered(NodeOperation *op, const rcti &render_rect)
{
  auto &data = getOutputData(op);
  for (auto reg_rect : data.render_rects) {
    if (BLI_rcti_inside_rcti(&reg_rect, &render_rect)) {
      return true;
    }
  }
  return false;
}

void OutputManager::registerRender(NodeOperation *op, const rcti &render_rect)
{
  auto &data = getOutputData(op);
  data.render_rects.append(render_rect);
}

bool OutputManager::hasRegisteredReads(NodeOperation *op)
{
  return getOutputData(op).registered_reads > 0;
}

void OutputManager::registerRead(NodeOperation *op)
{
  auto &data = getOutputData(op);
  data.registered_reads++;
}

blender::Span<rcti> OutputManager::getRectsToRender(NodeOperation *op)
{
  return getOutputData(op).render_rects.as_span();
}

bool OutputManager::isOutputRendered(NodeOperation *op)
{
  return getOutputData(op).buffer != nullptr;
}

void OutputManager::giveRenderedOutput(NodeOperation *op, CPUBufferUniquePtr<float> buf)
{
  auto &data = getOutputData(op);
  data.buffer = m_cpu_man->asBaseBuffer(std::move(buf));
  BLI_assert(data.received_reads == 0);
}

void OutputManager::giveRenderedOutput(NodeOperation *op, GPUBufferUniquePtr buf)
{
  auto &data = getOutputData(op);
  data.buffer = m_gpu_man->asBaseBuffer(std::move(buf));
  BLI_assert(data.received_reads == 0);
}

const CPUBuffer<float> *OutputManager::getOutputCPU(NodeOperation *op)
{
  BLI_assert(isOutputRendered(op));
  auto &data = getOutputData(op);
  auto cpu_buf_uptr = m_cpu_man->adaptImageBuffer(std::move(data.buffer));
  auto cpu_buf = cpu_buf_uptr.get();
  data.buffer = m_cpu_man->asBaseBuffer(std::move(cpu_buf_uptr));
  return cpu_buf;
}

const GPUBuffer *OutputManager::getOutputGPU(NodeOperation *op)
{
  BLI_assert(isOutputRendered(op));
  auto &data = getOutputData(op);
  auto gpu_buf_uptr = m_gpu_man->adaptImageBuffer(std::move(data.buffer));
  auto gpu_buf = gpu_buf_uptr.get();
  data.buffer = m_gpu_man->asBaseBuffer(std::move(gpu_buf_uptr));
  return gpu_buf;
}

void OutputManager::reportRead(NodeOperation *read_op)
{
  auto &data = getOutputData(read_op);
  data.received_reads++;
  BLI_assert(data.received_reads > 0 && data.received_reads <= data.registered_reads);
  if (data.received_reads == data.registered_reads) {
    /* delete buffer */
    data.buffer = nullptr;
  }
}

}  // namespace blender::compositor