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

#include "COM_KeyingBlurOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
KeyingBlurOperation::KeyingBlurOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::VALUE);

  this->m_size = 0;
  this->m_axis = BLUR_AXIS_X;
}

void KeyingBlurOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_size);
  hashParam(m_axis);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel keyingBlurXOp(CCL_WRITE(dst), CCL_READ(input), int input_width, int size)
{
  READ_DECL(input);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  float average = 0.0f;
  const int start_x = max(0, dst_coords.x - size + 1);
  const int end_x = min(input_width, dst_coords.x + size);
  int count = end_x - start_x;
  SET_COORDS(input, start_x, dst_coords.y);
  while (input_coords.x < end_x) {
    READ_IMG1(input, input_pix);
    average += input_pix.x;
    INCR1_COORDS_X(input);
  }

  input_pix.x = average / (float)count;

  WRITE_IMG1(dst, input_pix);

  CPU_LOOP_END
}

ccl_kernel keyingBlurYOp(CCL_WRITE(dst), CCL_READ(input), int input_height, int size)
{
  READ_DECL(input);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  float average = 0.0f;
  const int start_y = max(0, dst_coords.y - size + 1);
  const int end_y = min(input_height, dst_coords.y + size);
  int count = end_y - start_y;
  SET_COORDS(input, dst_coords.x, start_y);
  while (input_coords.y < end_y) {
    READ_IMG1(input, input_pix);
    average += input_pix.x;
    INCR1_COORDS_Y(input);
  }

  input_pix.x = average / (float)count;

  WRITE_IMG1(dst, input_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void KeyingBlurOperation::execPixels(ExecutionManager &man)
{
  auto input = getInputOperation(0)->getPixels(this, man);
  int width = getWidth();
  int height = getHeight();
  BLI_assert(m_axis == BLUR_AXIS_X || m_axis == BLUR_AXIS_Y);
  if (m_axis == BLUR_AXIS_X) {
    std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
        CCL::keyingBlurXOp, _1, input, width, m_size);
    computeWriteSeek(man, cpu_write, "keyingBlurXOp", [&](ComputeKernel *kernel) {
      kernel->addReadImgArgs(*input);
      kernel->addIntArg(width);
      kernel->addIntArg(m_size);
    });
  }
  else {
    std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
        CCL::keyingBlurYOp, _1, input, height, m_size);
    computeWriteSeek(man, cpu_write, "keyingBlurYOp", [&](ComputeKernel *kernel) {
      kernel->addReadImgArgs(*input);
      kernel->addIntArg(height);
      kernel->addIntArg(m_size);
    });
  }
}
