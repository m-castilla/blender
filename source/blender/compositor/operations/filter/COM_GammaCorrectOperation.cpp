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

#include "COM_GammaCorrectOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
GammaCorrectOperation::GammaCorrectOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel gammaCorrectOp(CCL_WRITE(dst), CCL_READ(color))
{
  float3 zero_f3 = make_float3_1(0.0f);
  READ_DECL(color);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(color, dst_coords);

  READ_IMG4(color, color_pix);
  float3 rgb_result = make_float3_4(color_pix);
  if (color_pix.w > 0.0f) {
    rgb_result /= color_pix.w;
  }

  /* check for negative */
  rgb_result = select(zero_f3, rgb_result * rgb_result, rgb_result > zero_f3);

  if (color_pix.w > 0.0f) {
    rgb_result *= color_pix.w;
  }

  float4 result_pix = make_float4_3(rgb_result);
  result_pix.w = color_pix.w;
  WRITE_IMG4(dst, result_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void GammaCorrectOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::gammaCorrectOp, _1, color);
  return computeWriteSeek(man, cpu_write, "gammaCorrectOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
  });
}

GammaUncorrectOperation::GammaUncorrectOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel gammaUncorrectOp(CCL_WRITE(dst), CCL_READ(color))
{
  READ_DECL(color);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(color, dst_coords);

  READ_IMG4(color, color_pix);
  float3 rgb_result = make_float3_4(color_pix);
  if (color_pix.w > 0.0f) {
    rgb_result /= color_pix.w;
  }

  /* check for negative to avoid nan's */
  rgb_result.x = rgb_result.x > 0.0f ? sqrtf(rgb_result.x) : 0.0f;
  rgb_result.y = rgb_result.y > 0.0f ? sqrtf(rgb_result.y) : 0.0f;
  rgb_result.z = rgb_result.z > 0.0f ? sqrtf(rgb_result.z) : 0.0f;

  if (color_pix.w > 0.0f) {
    rgb_result *= color_pix.w;
  }

  float4 result_pix = make_float4_3(rgb_result);
  result_pix.w = color_pix.w;
  WRITE_IMG4(dst, result_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void GammaUncorrectOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::gammaUncorrectOp, _1, color);
  return computeWriteSeek(man, cpu_write, "gammaUncorrectOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
  });
}
