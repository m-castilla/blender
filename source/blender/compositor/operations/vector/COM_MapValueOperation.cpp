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

#include "COM_MapValueOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
MapValueOperation::MapValueOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::VALUE);
}

void MapValueOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_min);
  hashParam(m_max);
  hashParam(m_loc);
  hashParam(m_size);
  hashParam(m_flag);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel mapValueOp(
    CCL_WRITE(dst), CCL_READ(input), float min, float max, float size, float loc, int flag)
{
  READ_DECL(input);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(input, dst_coords);

  READ_IMG1(input, input_pix);

  input_pix.x = (input_pix.x + loc) * size;
  if (flag & TEXMAP_CLIP_MIN) {
    if (input_pix.x < min) {
      input_pix.x = min;
    }
  }
  if (flag & TEXMAP_CLIP_MAX) {
    if (input_pix.x > max) {
      input_pix.x = max;
    }
  }

  WRITE_IMG1(dst, input_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void MapValueOperation::execPixels(ExecutionManager &man)
{
  auto input = getInputOperation(0)->getPixels(this, man);
  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::mapValueOp, _1, input, m_min, m_max, m_size, m_loc, m_flag);
  computeWriteSeek(man, cpu_write, "mapValueOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input);
    kernel->addFloatArg(m_min);
    kernel->addFloatArg(m_max);
    kernel->addFloatArg(m_size);
    kernel->addFloatArg(m_loc);
    kernel->addBoolArg(m_flag);
  });
}