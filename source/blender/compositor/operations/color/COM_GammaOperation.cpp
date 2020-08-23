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

#include "COM_GammaOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
GammaOperation::GammaOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel gammaOp(CCL_WRITE(dst), CCL_READ(color), CCL_READ(gamma))
{
  READ_DECL(gamma);
  READ_DECL(color);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(gamma, dst_coords);
  COPY_COORDS(color, dst_coords);

  READ_IMG(gamma, gamma_pix);
  READ_IMG(color, color_pix);

  /* check for negative to avoid nan's */
  color_pix.x = color_pix.x > 0.0f ? powf(color_pix.x, gamma_pix.x) : color_pix.x;
  color_pix.y = color_pix.y > 0.0f ? powf(color_pix.y, gamma_pix.x) : color_pix.y;
  color_pix.z = color_pix.z > 0.0f ? powf(color_pix.z, gamma_pix.x) : color_pix.z;

  WRITE_IMG(dst, color_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void GammaOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto gamma = getInputOperation(1)->getPixels(this, man);
  auto cpu_write = std::bind(CCL::gammaOp, _1, color, gamma);
  computeWriteSeek(man, cpu_write, "gammaOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*gamma);
  });
}
