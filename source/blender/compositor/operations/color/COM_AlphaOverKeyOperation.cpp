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

#include "COM_AlphaOverKeyOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel alphaOverKeyOp(CCL_WRITE(dst), CCL_READ(value), CCL_READ(color), CCL_READ(over_color))
{
  READ_DECL(value);
  READ_DECL(color);
  READ_DECL(over_color);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  READ_COORDS_TO_OFFSET(value, dst);
  READ_COORDS_TO_OFFSET(color, dst);
  READ_COORDS_TO_OFFSET(over_color, dst);
  WRITE_COORDS_TO_OFFSET(dst);

  READ_IMG(value, value_coords, value_pix);
  READ_IMG(color, color_coords, color_pix);
  READ_IMG(over_color, over_color_coords, over_color_pix);

  if (over_color_pix.w <= 0.0f) {
    WRITE_IMG(dst, dst_coords, color_pix);
  }
  else if (value_pix.x == 1.0f && over_color_pix.w >= 1.0f) {
    WRITE_IMG(dst, dst_coords, over_color_pix);
  }
  else {
    float premul = value_pix.x * over_color_pix.w;
    float premul_inv = 1.0f - premul;

    color_pix = premul_inv * color_pix;
    float alpha = color_pix.w;
    color_pix += premul * over_color_pix;
    color_pix.w = alpha + value_pix.x * over_color_pix.w;
    WRITE_IMG(dst, dst_coords, color_pix);
  }

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void AlphaOverKeyOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::alphaOverKeyOp, _1, value, color1, color2);
  computeWriteSeek(man, cpu_write, "alphaOverKeyOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
  });
}
