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

#include "COM_SetAlphaOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
SetAlphaOperation::SetAlphaOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel setAlphaOp(CCL_WRITE(dst), CCL_READ(color), CCL_READ(alpha))
{
  READ_DECL(color);
  READ_DECL(alpha);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(color, dst_coords);
  COPY_COORDS(alpha, dst_coords);

  READ_IMG4(color, color_pix);
  READ_IMG1(alpha, alpha_pix);
  color_pix.w = alpha_pix.x;
  WRITE_IMG4(dst, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void SetAlphaOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto alpha = getInputOperation(1)->getPixels(this, man);
  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::setAlphaOp, _1, color, alpha);
  computeWriteSeek(man, cpu_write, "setAlphaOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*alpha);
  });
}
