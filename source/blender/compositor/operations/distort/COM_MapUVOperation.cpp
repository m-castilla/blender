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

#include "COM_MapUVOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"

#include "COM_kernel_cpu.h"

using namespace std::placeholders;
MapUVOperation::MapUVOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR, InputResizeMode::NO_RESIZE);
  this->addInputSocket(SocketType::VECTOR);
  this->addOutputSocket(SocketType::COLOR);
  this->m_alpha = 0.0f;
  setMainInputSocketIndex(1);
}

void MapUVOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_alpha);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_device_inline bool read_uv(CCL_IMAGE(uv),
                               float x,
                               float y,
                               float uv_width,
                               float uv_height,
                               float color_width,
                               float color_height,
                               CCL_SAMPLER(bilinear_sampler),
                               float3 *r_vector)
{
  if (x < 0.0f || x >= uv_width || y < 0.0f || y >= uv_height) {
    *r_vector = make_float3_1(0.0f);
    return false;
  }

  float4 vector;
  float2 uv_coordsf = make_float2(x, y);
  SAMPLE_BILINEAR3_CLIP(0, uv, bilinear_sampler, vector);
  r_vector->x = vector.x * color_width;
  r_vector->y = vector.y * color_height;
  r_vector->z = vector.z;
  return true;
}

ccl_kernel mapUvOp(CCL_WRITE(dst),
                   CCL_READ(color),
                   CCL_READ(uv),
                   CCL_SAMPLER(nearest_sampler),
                   CCL_SAMPLER(bilinear_sampler),
                   const int uv_w,
                   const int uv_h,
                   const float alpha_set,
                   const int color_w,
                   const int color_h,
                   const float inv_color_w,
                   const float inv_color_h,
                   const float sqrt_color_w,
                   const float height_by_sqrt_color_w)
{
  READ_DECL(color);
  READ_DECL(uv);
  WRITE_DECL(dst);

  float3 main_uva, uva;  // u,v coordinates and alpha
  float2 derivative1;
  float2 derivative2;
  int num;

  CPU_LOOP_START(dst);

  read_uv(CCL_IMAGE_ARG(uv),
          dst_coords.x,
          dst_coords.y,
          uv_w,
          uv_h,
          color_w,
          color_h,
          bilinear_sampler,
          &main_uva);
  float main_alpha = main_uva.z;
  if (main_alpha == 0.0f) {
    WRITE_IMG4(dst, TRANSPARENT_PIXEL);
  }
  else {
    /* Estimate partial derivatives using 1-pixel offsets */
    const float epsilon[2] = {1.0f, 1.0f};

    derivative1 = ZERO_F2;
    derivative2 = ZERO_F2;
    num = 0;
    if (read_uv(CCL_IMAGE_ARG(uv),
                dst_coords.x + epsilon[0],
                dst_coords.y,
                uv_w,
                uv_h,
                color_w,
                color_h,
                bilinear_sampler,
                &uva)) {
      derivative1.x += uva.x - main_uva.x;
      derivative2.x += uva.y - main_uva.y;
      num++;
    }
    if (read_uv(CCL_IMAGE_ARG(uv),
                dst_coords.x - epsilon[0],
                dst_coords.y,
                uv_w,
                uv_h,
                color_w,
                color_h,
                bilinear_sampler,
                &uva)) {
      derivative1.x += main_uva.x - uva.x;
      derivative2.x += main_uva.y - uva.y;
      num++;
    }
    if (num > 0) {
      float numinv = 1.0f / (float)num;
      derivative1.x *= numinv;
      derivative2.x *= numinv;
    }

    num = 0;
    if (read_uv(CCL_IMAGE_ARG(uv),
                dst_coords.x,
                dst_coords.y + epsilon[1],
                uv_w,
                uv_h,
                color_w,
                color_h,
                bilinear_sampler,
                &uva)) {
      derivative1.y += uva.x - main_uva.x;
      derivative2.y += uva.y - main_uva.y;
      num++;
    }
    if (read_uv(CCL_IMAGE_ARG(uv),
                dst_coords.x,
                dst_coords.y - epsilon[1],
                uv_w,
                uv_h,
                color_w,
                color_h,
                bilinear_sampler,
                &uva)) {
      derivative1.y += main_uva.x - uva.x;
      derivative2.y += main_uva.y - uva.y;
      num++;
    }
    if (num > 0) {
      float numinv = 1.0f / (float)num;
      derivative1.y *= numinv;
      derivative2.y *= numinv;
    }

    /* EWA filtering */
    float2 main_uv = make_float2(main_uva.x, main_uva.y);
    EWA_FILTER_IMG(color,
                   nearest_sampler,
                   main_uv,
                   derivative1,
                   derivative2,
                   color_w,
                   color_h,
                   inv_color_w,
                   inv_color_h,
                   sqrt_color_w,
                   height_by_sqrt_color_w,
                   color_pix);

    /* UV to alpha threshold */
    const float threshold = alpha_set * 0.05f;
    /* XXX alpha threshold is used to fade out pixels on boundaries with invalid derivatives.
     * this calculation is not very well defined, should be looked into if it becomes a problem ...
     */
    float du = length(derivative1);
    float dv = length(derivative2);
    float factor = 1.0f - threshold * (du / color_w + dv / color_h);
    if (factor < 0.0f) {
      main_alpha = 0.0f;
    }
    else {
      main_alpha *= factor;
    }

    /* "premul" */
    if (main_alpha < 1.0f) {
      color_pix *= main_alpha;
    }

    WRITE_IMG4(dst, color_pix);
  }

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MapUVOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto uv = getInputOperation(1)->getPixels(this, man);
  int uv_w, uv_h, color_w, color_h;
  float inv_color_w, inv_color_h, sqrt_color_w, height_by_sqrt_color_w;
  if (man.canExecPixels()) {
    uv_w = uv->getWidth();
    uv_h = uv->getHeight();
    color_w = color->getWidth();
    color_h = color->getHeight();
    inv_color_w = 1.0f / color_w;
    inv_color_h = 1.0f / color_h;
    sqrt_color_w = sqrtf(color_w);
    height_by_sqrt_color_w = color_h / sqrt_color_w;
  }
  PixelsSampler nearest_sampler = PixelsSampler{PixelInterpolation::NEAREST, PixelExtend::CLIP};
  PixelsSampler bilinear_sampler = PixelsSampler{PixelInterpolation::BILINEAR, PixelExtend::CLIP};
  auto cpu_write = std::bind(CCL::mapUvOp,
                             _1,
                             color,
                             uv,
                             nearest_sampler,
                             bilinear_sampler,
                             uv_w,
                             uv_h,
                             m_alpha,
                             color_w,
                             color_h,
                             inv_color_w,
                             inv_color_h,
                             sqrt_color_w,
                             height_by_sqrt_color_w);
  computeWriteSeek(man, cpu_write, "mapUvOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*uv);
    kernel->addSamplerArg(nearest_sampler);
    kernel->addSamplerArg(bilinear_sampler);
    kernel->addIntArg(uv_w);
    kernel->addIntArg(uv_h);
    kernel->addBoolArg(m_alpha);
    kernel->addIntArg(color_w);
    kernel->addIntArg(color_h);
    kernel->addFloatArg(inv_color_w);
    kernel->addFloatArg(inv_color_h);
    kernel->addFloatArg(sqrt_color_w);
    kernel->addFloatArg(height_by_sqrt_color_w);
  });
}