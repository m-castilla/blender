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

#include "COM_DisplaceOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
DisplaceOperation::DisplaceOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::VECTOR);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_device_inline bool read_displacement(CCL_IMAGE(vector),
                                         CCL_SAMPLER(bilinear_sampler),
                                         const float x,
                                         const float y,
                                         const float xscale,
                                         const float yscale,
                                         const float2 origin,
                                         const int vector_w,
                                         const int vector_h,
                                         float2 *r_uv)
{
  if (x < 0.0f || x >= vector_w || y < 0.0f || y >= vector_h) {
    r_uv->x = 0.0f;
    r_uv->y = 0.0f;
    return false;
  }

  float4 vector_pix;
  float2 vector_coordsf = make_float2(x, y);
  SAMPLE_BILINEAR3_CLIP(0, vector, bilinear_sampler, vector_pix);
  r_uv->x = origin.x - vector_pix.x * xscale;
  r_uv->y = origin.y - vector_pix.y * yscale;
  return true;
}

ccl_kernel displaceOp(CCL_WRITE(dst),
                      CCL_READ(color),
                      CCL_READ(vector),
                      CCL_READ(scale_x),
                      CCL_READ(scale_y),
                      CCL_SAMPLER(nearest_sampler),
                      CCL_SAMPLER(bilinear_sampler),
                      const int width,
                      const int height,
                      const float width_x4,
                      const float height_x4,
                      const float inv_width,             // 1.0f / width
                      const float inv_height,            // 1.0f / height
                      const float sqrt_width,            // sqrtf(width)
                      const float height_by_sqrt_width)  // height / sqrtf(width))
{
  READ_DECL(color);
  READ_DECL(scale_x);
  READ_DECL(scale_y);
  READ_DECL(vector);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  float2 uv, main_uv;
  float2 derivative1, derivative2;

  /* clamp x and y displacement to quadruple image resolution -
   * to prevent hangs from huge values mistakenly plugged in eg. z buffers */
  COPY_COORDS(scale_x, dst_coords);
  READ_IMG1(scale_x, scale_x_pix);
  const float xs = clamp(scale_x_pix.x, -width_x4, width_x4);
  COPY_COORDS(scale_y, dst_coords);
  READ_IMG1(scale_y, scale_y_pix);
  const float ys = clamp(scale_y_pix.x, -height_x4, height_x4);

  /* displaced pixel in uv coords, for image sampling */
  const float2 dst_coordsf = make_float2(dst_coords.x, dst_coords.y);
  read_displacement(CCL_IMAGE_ARG(vector),
                    bilinear_sampler,
                    dst_coordsf.x,
                    dst_coordsf.y,
                    xs,
                    ys,
                    dst_coordsf,
                    width,
                    height,
                    &main_uv);

  /* Estimate partial derivatives using 1-pixel offsets */
  const float2 epsilon = make_float2(1.0f, 1.0f);

  derivative1 = make_float2(0.0f, 0.0f);
  derivative2 = make_float2(0.0f, 0.0f);
  int num = 0;
  if (read_displacement(CCL_IMAGE_ARG(vector),
                        bilinear_sampler,
                        dst_coordsf.x + epsilon.x,
                        dst_coordsf.y,
                        xs,
                        ys,
                        dst_coordsf,
                        width,
                        height,
                        &uv)) {
    derivative1.x += uv.x - main_uv.x;
    derivative2.x += uv.y - main_uv.y;
    num++;
  }
  if (read_displacement(CCL_IMAGE_ARG(vector),
                        bilinear_sampler,
                        dst_coordsf.x - +epsilon.x,
                        dst_coordsf.y,
                        xs,
                        ys,
                        dst_coordsf,
                        width,
                        height,
                        &uv)) {
    derivative1.x += main_uv.x - uv.x;
    derivative2.x += main_uv.y - uv.y;
    num++;
  }
  if (num > 0) {
    float numinv = 1.0f / (float)num;
    derivative1.x *= numinv;
    derivative2.x *= numinv;
  }

  num = 0;
  if (read_displacement(CCL_IMAGE_ARG(vector),
                        bilinear_sampler,
                        dst_coordsf.x,
                        dst_coordsf.y + epsilon.y,
                        xs,
                        ys,
                        dst_coordsf,
                        width,
                        height,
                        &uv)) {
    derivative1.y += uv.x - main_uv.x;
    derivative2.y += uv.y - main_uv.y;
    num++;
  }
  if (read_displacement(CCL_IMAGE_ARG(vector),
                        bilinear_sampler,
                        dst_coordsf.x,
                        dst_coordsf.y - epsilon.y,
                        xs,
                        ys,
                        dst_coordsf,
                        width,
                        height,
                        &uv)) {
    derivative1.y += main_uv.x - uv.x;
    derivative2.y += main_uv.y - uv.y;
    num++;
  }
  if (num > 0) {
    float numinv = 1.0f / (float)num;
    derivative1.y *= numinv;
    derivative2.y *= numinv;
  }
  if (equals_f2(derivative1, ZERO_F2) && equals_f2(derivative2, ZERO_F2)) {
    COPY_SAMPLE_COORDS(color, main_uv);
    SAMPLE_BILINEAR4_CLIP(0, color, bilinear_sampler, color_pix);
  }
  else {
    /* EWA filtering (without nearest it gets blurry with NO distortion) */
    EWA_FILTER_IMG(color,
                   nearest_sampler,
                   main_uv,
                   derivative1,
                   derivative2,
                   width,
                   height,
                   inv_width,
                   inv_height,
                   sqrt_width,
                   height_by_sqrt_width,
                   color_pix);
  }

  WRITE_IMG4(dst, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void DisplaceOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto vector = getInputOperation(1)->getPixels(this, man);
  auto scale_x = getInputOperation(2)->getPixels(this, man);
  auto scale_y = getInputOperation(3)->getPixels(this, man);
  float width = getWidth();
  float height = getHeight();
  float width_x4 = width * 4.0f;
  float height_x4 = height * 4.0f;
  float inv_width = 1.0f / width;
  float inv_height = 1.0f / height;
  float sqrt_width = sqrtf(width);
  float height_by_sqrt_width = height / sqrt_width;
  PixelsSampler nearest_sampler = PixelsSampler{PixelInterpolation::NEAREST, PixelExtend::CLIP};
  PixelsSampler bilinear_sampler = PixelsSampler{PixelInterpolation::BILINEAR, PixelExtend::CLIP};
  auto cpu_write = std::bind(CCL::displaceOp,
                             _1,
                             color,
                             vector,
                             scale_x,
                             scale_y,
                             nearest_sampler,
                             bilinear_sampler,
                             width,
                             height,
                             width_x4,
                             height_x4,
                             inv_width,
                             inv_height,
                             sqrt_width,
                             height_by_sqrt_width);
  computeWriteSeek(man, cpu_write, "displaceOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*vector);
    kernel->addReadImgArgs(*scale_x);
    kernel->addReadImgArgs(*scale_y);
    kernel->addSamplerArg(nearest_sampler);
    kernel->addSamplerArg(bilinear_sampler);
    kernel->addIntArg(width);
    kernel->addIntArg(height);
    kernel->addFloatArg(width_x4);
    kernel->addFloatArg(height_x4);
    kernel->addFloatArg(inv_width);
    kernel->addFloatArg(inv_height);
    kernel->addFloatArg(sqrt_width);
    kernel->addFloatArg(height_by_sqrt_width);
  });
}