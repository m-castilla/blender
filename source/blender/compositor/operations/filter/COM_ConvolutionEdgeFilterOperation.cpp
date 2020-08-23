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

#include "COM_ConvolutionEdgeFilterOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
ConvolutionEdgeFilterOperation::ConvolutionEdgeFilterOperation() : ConvolutionFilterOperation()
{
  /* pass */
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convolutionEdgeFilterOp(CCL_WRITE(dst),
                                   CCL_READ(color),
                                   CCL_READ(value),
                                   ccl_constant float3 *filter,
                                   const int3 last_x,
                                   const int3 last_y)
{
  READ_DECL(color);
  READ_DECL(value);
  WRITE_DECL(dst);
  float alpha;
  float3 middle_rgb;
  float3 rgb_pix, rgb1, rgb2;
  int3 x_coords, y_coords;
  int3 zero_i3 = make_int3_1(0);

  CPU_LOOP_START(dst);

  x_coords.x = dst_coords.x - 1;
  x_coords.y = dst_coords.x;
  x_coords.z = dst_coords.x + 1;
  y_coords.x = dst_coords.y - 1;
  y_coords.y = dst_coords.y;
  y_coords.z = dst_coords.y + 1;
  x_coords = clamp(x_coords, zero_i3, last_x);
  y_coords = clamp(y_coords, zero_i3, last_y);

  /* read middle coords*/
  SET_COORDS(value, x_coords.y, y_coords.y);
  READ_IMG1(value, value_pix);
  const float mval = 1.0f - value_pix.x;

  COPY_COORDS(color, value_coords);
  READ_IMG4(color, color_pix);
  alpha = color_pix.w;
  middle_rgb = make_float3_4(color_pix);

  /* read others coords */
  SET_COORDS(color, x_coords.x, y_coords.x);
  READ_IMG4(color, color_pix);
  rgb_pix = make_float3_4(color_pix);
  rgb1 = rgb_pix * filter[0];
  rgb2 = rgb1;

  UPDATE_COORDS_X(color, x_coords.y);
  READ_IMG4(color, color_pix);
  rgb_pix = make_float3_4(color_pix);
  rgb1 += rgb_pix * filter[1];
  rgb2 += rgb_pix * filter[3];

  UPDATE_COORDS_X(color, x_coords.z);
  READ_IMG4(color, color_pix);
  rgb_pix = make_float3_4(color_pix);
  rgb1 += rgb_pix * filter[2];
  rgb2 += rgb_pix * filter[6];

  SET_COORDS(color, x_coords.x, y_coords.y);
  READ_IMG4(color, color_pix);
  rgb_pix = make_float3_4(color_pix);
  rgb1 += rgb_pix * filter[3];
  rgb2 += rgb_pix * filter[1];

  rgb_pix = middle_rgb * filter[4];
  rgb1 += rgb_pix;
  rgb2 += rgb_pix;

  UPDATE_COORDS_X(color, x_coords.z);
  READ_IMG4(color, color_pix);
  rgb_pix = make_float3_4(color_pix);
  rgb1 += rgb_pix * filter[5];
  rgb2 += rgb_pix * filter[7];

  SET_COORDS(color, x_coords.x, y_coords.z);
  READ_IMG4(color, color_pix);
  rgb_pix = make_float3_4(color_pix);
  rgb1 += rgb_pix * filter[6];
  rgb2 += rgb_pix * filter[2];

  UPDATE_COORDS_X(color, x_coords.y);
  READ_IMG4(color, color_pix);
  rgb_pix = make_float3_4(color_pix);
  rgb1 += rgb_pix * filter[7];
  rgb2 += rgb_pix * filter[5];

  UPDATE_COORDS_X(color, x_coords.z);
  READ_IMG4(color, color_pix);
  rgb_pix = make_float3_4(color_pix) * filter[8];
  rgb1 += rgb_pix;
  rgb2 += rgb_pix;

  rgb_pix = sqrt(rgb1 * rgb1 + rgb2 * rgb2);
  rgb_pix = rgb_pix * value_pix.x + middle_rgb * mval;
  color_pix = make_float4_3(rgb_pix);
  color_pix.w = alpha;

  /* Make sure we don't return negative color. */
  color_pix = max(color_pix, 0.0f);

  WRITE_IMG4(dst, color_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvolutionEdgeFilterOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto value = getInputOperation(1)->getPixels(this, man);
  CCL::int3 last_x = CCL::make_int3_1(getWidth() - 1);
  CCL::int3 last_y = CCL::make_int3_1(getHeight() - 1);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::convolutionEdgeFilterOp, _1, color, value, m_filter_f3, last_x, last_y);
  computeWriteSeek(man, cpu_write, "convolutionEdgeFilterOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*value);
    kernel->addFloat3CArrayArg(m_filter_f3, COM_CONVOLUTION_FILTER_SIZE);
    kernel->addInt3Arg(last_x);
    kernel->addInt3Arg(last_y);
  });
}
