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

#include "COM_ProjectorLensDistortionOperation.h"
#include "BLI_math.h"
#include "BLI_utildefines.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
ProjectorLensDistortionOperation::ProjectorLensDistortionOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel projectorLensDistortionOp(CCL_WRITE(dst),
                                     CCL_READ(color),
                                     CCL_SAMPLER(bilinear_sampler),
                                     const int width,
                                     const int height,
                                     const float kr2)
{
  READ_DECL(color);
  WRITE_DECL(dst);
  float4 result;

  CPU_LOOP_START(dst);

  const float start_u = (dst_coords.x + 0.5f) / width;
  const float start_v = (dst_coords.y + 0.5f) / height;

  float u = (start_u * width + kr2) - 0.5f;
  const float v = start_v * height - 0.5f;
  SET_SAMPLE_COORDS(color, u, v);
  SAMPLE_BILINEAR4_CLIP(0, color, bilinear_sampler, color_pix);
  result.x = color_pix.x;

  COPY_COORDS(color, dst_coords);
  READ_IMG4(color, color_pix);
  result.y = color_pix.y;

  u = (start_u * width - kr2) - 0.5f;
  UPDATE_SAMPLE_COORDS_X(color, u);
  SAMPLE_BILINEAR4_CLIP(0, color, bilinear_sampler, color_pix);
  result.z = color_pix.z;

  result.w = 1.0f;

  WRITE_IMG4(dst, result);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ProjectorLensDistortionOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto dispersion_pixel = getInputOperation(1)->getSinglePixel(this, man, 0, 0);

  float dispersion = 0.0f;
  if (dispersion_pixel != nullptr) {
    dispersion = *dispersion_pixel;
  }
  float kr = 0.25f * max_ff(min_ff(dispersion, 1.0f), 0.0f);
  float kr2 = kr * 20;

  float width = getWidth();
  float height = getHeight();
  PixelsSampler bilinear_sampler = PixelsSampler{PixelInterpolation::BILINEAR, PixelExtend::CLIP};
  auto cpu_write = std::bind(
      CCL::projectorLensDistortionOp, _1, color, bilinear_sampler, width, height, kr2);
  computeWriteSeek(man, cpu_write, "projectorLensDistortionOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addSamplerArg(bilinear_sampler);
    kernel->addIntArg(width);
    kernel->addIntArg(height);
    kernel->addFloatArg(kr2);
  });
}