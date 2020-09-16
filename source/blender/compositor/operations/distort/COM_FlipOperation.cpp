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

#include "COM_FlipOperation.h"
#include "COM_ComputeKernel.h"

#include "COM_kernel_cpu.h"

using namespace std::placeholders;
FlipOperation::FlipOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::DYNAMIC);
  this->addOutputSocket(SocketType::DYNAMIC);
  this->m_flipX = true;
  this->m_flipY = false;
}
void FlipOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_flipX);
  hashParam(m_flipY);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel flipOp(
    CCL_WRITE(dst), CCL_READ(input), const int width, const int height, BOOL flip_x, BOOL flip_y)
{
  READ_DECL(input);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  input_coords.x = flip_x ? (width - 1) - dst_coords.x : dst_coords.x;
  input_coords.y = flip_y ? (height - 1) - dst_coords.y : dst_coords.y;

  SET_COORDS(input, input_coords.x, input_coords.y);
  READ_IMG(input, input_pix);

  WRITE_IMG(dst, input_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void FlipOperation::execPixels(ExecutionManager &man)
{
  auto input = getInputOperation(0)->getPixels(this, man);
  float width = getWidth();
  float height = getHeight();
  auto cpu_write = std::bind(CCL::flipOp, _1, input, width, height, m_flipX, m_flipY);
  computeWriteSeek(man, cpu_write, "flipOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input);
    kernel->addIntArg(width);
    kernel->addIntArg(height);
    kernel->addBoolArg(m_flipX);
    kernel->addBoolArg(m_flipY);
  });
}