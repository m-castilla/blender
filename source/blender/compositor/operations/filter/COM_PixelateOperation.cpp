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

#include "COM_PixelateOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"
#include <algorithm>

using namespace std::placeholders;
PixelateOperation::PixelateOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::DYNAMIC);
  this->addOutputSocket(SocketType::DYNAMIC);
  m_size = 0.0f;
}

void PixelateOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_size);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel pixelateOp(CCL_WRITE(dst), CCL_READ(input), float size)
{
  READ_DECL(input);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  int down_x = dst_coords.x * size;
  int down_y = dst_coords.y * size;
  int up_x = down_x * (1.0f / size);
  int up_y = down_y * (1.0f / size);
  SET_COORDS(input, up_x, up_y);
  READ_IMG(input, input_pix);
  WRITE_IMG(dst, input_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void PixelateOperation::execPixels(ExecutionManager &man)
{
  auto input = getInputOperation(0)->getPixels(this, man);
  float size = 1.0f - m_size;
  if (size <= 0) {
    size = FLT_EPSILON;
  }
  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::pixelateOp, _1, input, size);
  computeWriteSeek(man, cpu_write, "pixelateOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input);
    kernel->addFloatArg(size);
  });
}
