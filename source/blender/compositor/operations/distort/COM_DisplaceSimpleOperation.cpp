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
 * Copyright 2012, Blender Foundation.
 */

#include "COM_DisplaceSimpleOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
DisplaceSimpleOperation::DisplaceSimpleOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::VECTOR);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel displaceSimpleOp(CCL_WRITE(dst),
                            CCL_READ(color),
                            CCL_READ(vector),
                            CCL_READ(scale_x),
                            CCL_READ(scale_y),
                            const int width,
                            const int height,
                            const float width_x4,
                            const float height_x4)
{
  READ_DECL(color);
  READ_DECL(scale_x);
  READ_DECL(scale_y);
  READ_DECL(vector);
  WRITE_DECL(dst);

  float p_dx, p_dy; /* main displacement in pixel space */
  float u, v;

  CPU_LOOP_START(dst);

  COPY_COORDS(scale_x, dst_coords);
  COPY_COORDS(scale_y, dst_coords);
  READ_IMG1(scale_x, scale_x_pix);
  READ_IMG1(scale_y, scale_y_pix);

  /* clamp x and y displacement to triple image resolution -
   * to prevent hangs from huge values mistakenly plugged in eg. z buffers */
  const float xs = clamp(scale_x_pix.x, -width_x4, width_x4);
  const float ys = clamp(scale_y_pix.x, -height_x4, height_x4);

  COPY_COORDS(vector, dst_coords);
  READ_IMG3(vector, vector_pix);
  p_dx = vector_pix.x * xs;
  p_dy = vector_pix.y * ys;

  /* displaced pixel in uv coords, for image sampling */
  /* clamp nodes to avoid glitches */
  u = dst_coords.x - p_dx + 0.5f;
  v = dst_coords.y - p_dy + 0.5f;
  u = clamp(u, 0.0f, width - 1.0f);
  v = clamp(v, 0.0f, height - 1.0f);

  SET_COORDS(color, u, v);
  READ_IMG4(color, color_pix);

  WRITE_IMG4(dst, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void DisplaceSimpleOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto vector = getInputOperation(1)->getPixels(this, man);
  auto scale_x = getInputOperation(2)->getPixels(this, man);
  auto scale_y = getInputOperation(3)->getPixels(this, man);
  float width = getWidth();
  float height = getHeight();
  float width_x4 = width * 4.0f;
  float height_x4 = height * 4.0f;
  auto cpu_write = std::bind(CCL::displaceSimpleOp,
                             _1,
                             color,
                             vector,
                             scale_x,
                             scale_y,
                             width,
                             height,
                             width_x4,
                             height_x4);
  computeWriteSeek(man, cpu_write, "displaceSimpleOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*vector);
    kernel->addReadImgArgs(*scale_x);
    kernel->addReadImgArgs(*scale_y);
    kernel->addIntArg(width);
    kernel->addIntArg(height);
    kernel->addFloatArg(width_x4);
    kernel->addFloatArg(height_x4);
  });
}