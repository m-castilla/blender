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

#include "COM_DespeckleOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
DespeckleOperation::DespeckleOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::COLOR);
  this->setMainInputSocketIndex(0);
  m_threshold = 0.0f;
  m_threshold_neighbor = 0.0f;
}

void DespeckleOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_threshold);
  hashParam(m_threshold_neighbor);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel despeckleOp(CCL_WRITE(dst),
                       CCL_READ(color),
                       CCL_READ(value),
                       int3 last_x,
                       int3 last_y,
                       const float threshold,
                       const float threshold_neighbor)
{
#define TOT_DIV_ONE 1.0f
#define TOT_DIV_CNR M_SQRT1_2_F
#define FACTOR_TOT (TOT_DIV_ONE * 4 + TOT_DIV_CNR * 4)

  READ_DECL(color);
  READ_DECL(value);
  WRITE_DECL(dst);

  float factor_accum;
  float4 middle_color;
  float4 color_pix2;
  float4 color_accum;
  float4 color_accum_ok;
  float4 tot_div_one_f4 = make_float4_1(TOT_DIV_ONE);
  float4 tot_div_cnr_f4 = make_float4_1(TOT_DIV_CNR);
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

  COPY_COORDS(color, value_coords);
  READ_IMG4(color, middle_color);

  color_accum = make_float4_1(0.0f);
  color_accum_ok = make_float4_1(0.0f);
  factor_accum = 0.0f;

#define COLOR_READ_AND_ADD(factor_f4) \
  { \
    READ_IMG4(color, color_pix); \
    color_pix2 = color_pix * factor_f4; \
    color_accum += color_pix2; \
    if (color_diff_threshold(color_pix, middle_color, threshold)) { \
      factor_accum += factor_f4.x; \
      color_accum_ok += color_pix2; \
    } \
  }

  SET_COORDS(color, x_coords.x, y_coords.x);
  COLOR_READ_AND_ADD(tot_div_cnr_f4);

  UPDATE_COORDS_X(color, x_coords.y);
  COLOR_READ_AND_ADD(tot_div_one_f4);

  UPDATE_COORDS_X(color, x_coords.z);
  COLOR_READ_AND_ADD(tot_div_cnr_f4);

  SET_COORDS(color, x_coords.x, y_coords.y);
  COLOR_READ_AND_ADD(tot_div_one_f4);

  UPDATE_COORDS_X(color, x_coords.z);
  COLOR_READ_AND_ADD(tot_div_one_f4);

  SET_COORDS(color, x_coords.x, y_coords.z);
  COLOR_READ_AND_ADD(tot_div_cnr_f4);

  UPDATE_COORDS_X(color, x_coords.y);
  COLOR_READ_AND_ADD(tot_div_one_f4);

  UPDATE_COORDS_X(color, x_coords.z);
  COLOR_READ_AND_ADD(tot_div_cnr_f4);

  color_accum *= (1.0f / (4.0f + (4.0f * M_SQRT1_2_F)));
  // mul_v4_fl(color_mid, 1.0f / w);

  if ((factor_accum != 0.0f) && ((factor_accum / FACTOR_TOT) > (threshold_neighbor)) &&
      color_diff_threshold(color_accum, middle_color, threshold)) {
    color_accum_ok *= (1.0f / factor_accum);
    color_pix = interp_f4f4(middle_color, color_accum_ok, value_pix.x);
    WRITE_IMG4(dst, color_pix);
  }
  else {
    WRITE_IMG4(dst, middle_color);
  }

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void DespeckleOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto value = getInputOperation(1)->getPixels(this, man);
  CCL::int3 last_x = CCL::make_int3_1(getWidth() - 1);
  CCL::int3 last_y = CCL::make_int3_1(getHeight() - 1);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::despeckleOp, _1, color, value, last_x, last_y, m_threshold, m_threshold_neighbor);
  computeWriteSeek(man, cpu_write, "despeckleOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*value);
    kernel->addInt3Arg(last_x);
    kernel->addInt3Arg(last_y);
    kernel->addFloatArg(m_threshold);
    kernel->addFloatArg(m_threshold_neighbor);
  });
}
