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

#include "COM_InvertOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
InvertOperation::InvertOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
  this->m_color = true;
  this->m_alpha = false;
  setMainInputSocketIndex(1);
}

void InvertOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_color);
  hashParam(m_alpha);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel invertOp(
    CCL_WRITE(dst), CCL_READ(factor), CCL_READ(color), BOOL do_color, BOOL do_alpha)
{
  READ_DECL(factor);
  READ_DECL(color);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(factor, dst_coords);
  COPY_COORDS(color, dst_coords);

  READ_IMG1(factor, factor_pix);
  READ_IMG4(color, color_pix);

  const float value = factor_pix.x;
  const float inv_value = 1.0f - value;
  const float alpha = color_pix.w;
  if (do_color) {
    color_pix = (1.0f - color_pix) * value + color_pix * inv_value;
  }

  if (do_alpha) {
    color_pix.w = (1.0f - alpha) * value + alpha * inv_value;
  }
  else {
    color_pix.w = alpha;
  }

  WRITE_IMG(dst, color_pix);

  CPU_LOOP_END;
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void InvertOperation::execPixels(ExecutionManager &man)
{
  auto value = getInputOperation(0)->getPixels(this, man);
  auto color = getInputOperation(1)->getPixels(this, man);
  auto cpu_write = std::bind(CCL::invertOp, _1, value, color, m_color, m_alpha);
  computeWriteSeek(man, cpu_write, "invertOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color);
    kernel->addBoolArg(m_color);
    kernel->addBoolArg(m_alpha);
  });
}