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

#include "COM_ChangeHSVOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_Pixels.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
ChangeHSVOperation::ChangeHSVOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel changeHsvOp(
    CCL_WRITE(dst), CCL_READ(color), CCL_READ(hue), CCL_READ(sat), CCL_READ(value))
{
  READ_DECL(value);
  READ_DECL(color);
  READ_DECL(hue);
  READ_DECL(sat);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(value, dst_coords, value_pix);
  READ_IMG(color, dst_coords, color_pix);
  READ_IMG(hue, dst_coords, hue_pix);
  READ_IMG(sat, dst_coords, sat_pix);

  color_pix.x += (hue_pix.x - 0.5f);
  if (color_pix.x > 1.0f) {
    color_pix.x -= 1.0f;
  }
  else if (color_pix.x < 0.0f) {
    color_pix.x += 1.0f;
  }
  color_pix.y *= sat_pix.x;
  color_pix.z *= value_pix.x;

  WRITE_IMG(dst, dst_coords, color_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void ChangeHSVOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto hue = getInputOperation(1)->getPixels(this, man);
  auto sat = getInputOperation(2)->getPixels(this, man);
  auto value = getInputOperation(3)->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::changeHsvOp, _1, color, hue, sat, value);
  computeWriteSeek(man, cpu_write, "changeHsvOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*hue);
    kernel->addReadImgArgs(*sat);
    kernel->addReadImgArgs(*value);
  });
}
