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

#include "COM_ConvolutionFilterOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
ConvolutionFilterOperation::ConvolutionFilterOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::COLOR);
  setMainInputSocketIndex(0);
}

void ConvolutionFilterOperation::hashParams()
{
  NodeOperation::hashParams();
  hashDataAsParam(m_filter, COM_CONVOLUTION_FILTER_SIZE);
}

void ConvolutionFilterOperation::set3x3Filter(
    float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9)
{
  this->m_filter[0] = f1;
  this->m_filter[1] = f2;
  this->m_filter[2] = f3;
  this->m_filter[3] = f4;
  this->m_filter[4] = f5;
  this->m_filter[5] = f6;
  this->m_filter[6] = f7;
  this->m_filter[7] = f8;
  this->m_filter[8] = f9;
  for (int i = 0; i < COM_CONVOLUTION_FILTER_SIZE; i++) {
    m_filter_f4[i] = CCL::make_float4_1(m_filter[i]);
    m_filter_f3[i] = CCL::make_float3_4(m_filter_f4[i]);
  }
  this->m_filterHeight = 3;
  this->m_filterWidth = 3;
  BLI_assert(m_filterHeight * m_filterWidth == COM_CONVOLUTION_FILTER_SIZE);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convolutionFilterOp(CCL_WRITE(dst),
                               CCL_READ(color),
                               CCL_READ(value),
                               ccl_constant float4 *filter,
                               const int3 last_x,
                               const int3 last_y)
{
  READ_DECL(color);
  READ_DECL(value);
  WRITE_DECL(dst);
  float4 middle_color_pix;
  float4 output_pix;
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

  output_pix = make_float4_1(0.0f);

  /* read middle coords*/
  SET_COORDS(value, x_coords.y, y_coords.y);
  READ_IMG1(value, value_pix);
  const float mval = 1.0f - value_pix.x;

  COPY_COORDS(color, value_coords)
  READ_IMG4(color, middle_color_pix);

  /* read others coords */
  SET_COORDS(color, x_coords.x, y_coords.x);
  READ_IMG4(color, color_pix);
  output_pix += color_pix * filter[0];

  UPDATE_COORDS_X(color, x_coords.y);
  READ_IMG4(color, color_pix);
  output_pix += color_pix * filter[1];

  UPDATE_COORDS_X(color, x_coords.z);
  READ_IMG4(color, color_pix);
  output_pix += color_pix * filter[2];

  SET_COORDS(color, x_coords.x, y_coords.y);
  READ_IMG4(color, color_pix);
  output_pix += color_pix * filter[3];

  output_pix += middle_color_pix * filter[4];

  UPDATE_COORDS_X(color, x_coords.z);
  READ_IMG4(color, color_pix);
  output_pix += color_pix * filter[5];

  SET_COORDS(color, x_coords.x, y_coords.z);
  READ_IMG4(color, color_pix);
  output_pix += color_pix * filter[6];

  UPDATE_COORDS_X(color, x_coords.y);
  READ_IMG4(color, color_pix);
  output_pix += color_pix * filter[7];

  UPDATE_COORDS_X(color, x_coords.z);
  READ_IMG4(color, color_pix);
  output_pix += color_pix * filter[8];

  output_pix = output_pix * value_pix.x + middle_color_pix * mval;

  /* Make sure we don't return negative color. */
  output_pix = max(output_pix, 0.0f);

  WRITE_IMG4(dst, output_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvolutionFilterOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto value = getInputOperation(1)->getPixels(this, man);
  CCL::int3 last_x = CCL::make_int3_1(getWidth() - 1);
  CCL::int3 last_y = CCL::make_int3_1(getHeight() - 1);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::convolutionFilterOp, _1, color, value, m_filter_f4, last_x, last_y);
  computeWriteSeek(man, cpu_write, "convolutionFilterOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*value);
    kernel->addFloat4CArrayArg(m_filter_f4, COM_CONVOLUTION_FILTER_SIZE);
    kernel->addInt3Arg(last_x);
    kernel->addInt3Arg(last_y);
  });
}
