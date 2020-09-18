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

#include "COM_ScreenLensDistortionOperation.h"
#include "BLI_math.h"
#include "BLI_utildefines.h"
#include "COM_ComputeKernel.h"
#include "COM_GlobalManager.h"
#include "COM_kernel_cpu.h"

ScreenLensDistortionOperation::ScreenLensDistortionOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::COLOR);
}

void ScreenLensDistortionOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_fit);
  hashParam(m_jitter);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_device_inline void distort_uv(
    const float2 uv, const int width, const int height, const float t, float2 *xy)
{
  float d = 1.0f / (1.0f + sqrtf(t));
  xy->x = (uv.x * d + 0.5f) * width - 0.5f;
  xy->y = (uv.y * d + 0.5f) * height - 0.5f;
}

ccl_device_inline bool get_delta(
    float r_sq, float k4, const float2 uv, const int width, const int height, float2 *delta)
{
  float t = 1.0f - k4 * r_sq;
  if (t >= 0.0f) {
    distort_uv(uv, width, height, t, delta);
    return true;
  }

  return false;
}

ccl_device_inline void accumulate(CCL_IMAGE(color),
                                  CCL_SAMPLER(bilinear_sampler),
                                  const int width,
                                  const int height,
                                  int a,
                                  int b,
                                  float r_sq,
                                  const float2 uv,
                                  const float2 delta[3],
                                  ccl_constant float k4_array[3],
                                  ccl_constant float dk4_array[3],
                                  float sum[4],
                                  int count[3],
                                  uint64_t *random_state,
                                  unsigned int *random_idx_state,
                                  int2 dst_coords,
                                  BOOL jitter)
{
  float4 color_pix;

  float dsf = distance(delta[a], delta[b]) + 1.0f;
  int ds = jitter ? (dsf < 4.0f ? 2 : (int)sqrtf(dsf)) : (int)dsf;
  float sd = 1.0f / (float)ds;

  float k4 = k4_array[a];
  float dk4 = dk4_array[a];

  for (float z = 0; z < ds; z++) {
    // printf("accumulate: %ul\n", *random_state);
    float tz = (z + (jitter ? random_float(random_state, random_idx_state, dst_coords) : 0.5f)) *
               sd;
    float t = 1.0f - (k4 + tz * dk4) * r_sq;
    // printf("after: %ul\n", *random_state);

    float2 color_coordsf;
    distort_uv(uv, width, height, t, &color_coordsf);
    SET_SAMPLE_COORDS(color, color_coordsf.x, color_coordsf.y);
    SAMPLE_BILINEAR4_CLIP(0, color, bilinear_sampler, color_pix);

    float *color_a = (float *)&color_pix;
    sum[a] += (1.0f - tz) * color_a[a];
    sum[b] += (tz)*color_a[b];
    count[a]++;
    count[b]++;
  }
}

ccl_kernel screenLensDistortOp(CCL_WRITE(dst),
                               CCL_READ(color),
                               CCL_SAMPLER(bilinear_sampler),
                               const int width,
                               const int height,
                               const float sc,
                               const float cx,
                               const float cy,
                               ccl_constant float k4[3],
                               ccl_constant float dk4[3],
                               uint64_t random_seed,
                               BOOL jitter)
{
  READ_DECL(color);
  WRITE_DECL(dst);
  CPU_LOOP_START(dst);

  float2 uv;
  uv.x = sc * ((dst_coords.x + 0.5f) - cx) / cx;
  uv.y = sc * ((dst_coords.y + 0.5f) - cy) / cy;
  float uv_dot = length_squared_f2(uv);

  int count[3] = {0, 0, 0};
  float2 delta[3];
  float sum[4] = {0, 0, 0, 0};

  bool valid_r = get_delta(uv_dot, k4[0], uv, width, height, &delta[0]);
  bool valid_g = get_delta(uv_dot, k4[1], uv, width, height, &delta[1]);
  bool valid_b = get_delta(uv_dot, k4[2], uv, width, height, &delta[2]);

  unsigned int random_idx_state = 0;
  if (valid_r && valid_g && valid_b) {
    int a = 0, b = 1;
    // printf("%ul\n", random_state);
    accumulate(CCL_IMAGE_ARG(color),
               bilinear_sampler,
               width,
               height,
               a,
               b,
               uv_dot,
               uv,
               delta,
               k4,
               dk4,
               sum,
               count,
               &random_seed,
               &random_idx_state,
               dst_coords,
               jitter);
    a = 1;
    b = 2;
    accumulate(CCL_IMAGE_ARG(color),
               bilinear_sampler,
               width,
               height,
               a,
               b,
               uv_dot,
               uv,
               delta,
               k4,
               dk4,
               sum,
               count,
               &random_seed,
               &random_idx_state,
               dst_coords,
               jitter);

    color_pix = BLACK_PIXEL;
    if (count[0]) {
      color_pix.x = 2.0f * sum[0] / (float)count[0];
    }
    if (count[1]) {
      color_pix.y = 2.0f * sum[1] / (float)count[1];
    }
    if (count[2]) {
      color_pix.z = 2.0f * sum[2] / (float)count[2];
    }

    WRITE_IMG(dst, color_pix);
  }
  else {
    WRITE_IMG(dst, TRANSPARENT_PIXEL);
  }

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

using namespace std::placeholders;
void ScreenLensDistortionOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto distortion_pix = getInputOperation(1)->getSinglePixel(this, man, 0, 0);
  auto dispersion_pix = getInputOperation(2)->getSinglePixel(this, man, 0, 0);

  float distortion = 0.0f;
  if (distortion_pix != nullptr) {
    distortion = *distortion_pix;
  }
  float dispersion = 0.0f;
  if (dispersion_pix != nullptr) {
    dispersion = *dispersion_pix;
  }

  int width = getWidth();
  int height = getHeight();

  float cx = 0.5f * (float)width;
  float cy = 0.5f * (float)height;

  float k[3];
  float dk4[3];
  float k4[3];
  k[1] = max_ff(min_ff(distortion, 1.0f), -0.999f);
  // smaller dispersion range for somewhat more control
  float d = 0.25f * max_ff(min_ff(dispersion, 1.0f), 0.0f);
  k[0] = max_ff(min_ff((k[1] + d), 1.0f), -0.999f);
  k[2] = max_ff(min_ff((k[1] - d), 1.0f), -0.999f);
  float maxk = max_fff(k[0], k[1], k[2]);
  float sc = (m_fit && (maxk > 0.0f)) ? (1.0f / (1.0f + 2.0f * maxk)) : (1.0f / (1.0f + maxk));
  dk4[0] = 4.0f * (k[1] - k[0]);
  dk4[1] = 4.0f * (k[2] - k[1]);
  dk4[2] = 0.0f; /* unused */

  mul_v3_v3fl(k4, k, 4.0f);

  auto bilinear_sampler = PixelsSampler{PixelInterpolation::BILINEAR, PixelExtend::CLIP};
  auto cpu_write = std::bind(CCL::screenLensDistortOp,
                             _1,
                             color,
                             bilinear_sampler,
                             width,
                             height,
                             sc,
                             cx,
                             cy,
                             k4,
                             dk4,
                             ComputeKernel::getRandomSeedArg(this),
                             m_jitter);
  computeWriteSeek(man, cpu_write, "screenLensDistortOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addSamplerArg(bilinear_sampler);
    kernel->addIntArg(width);
    kernel->addIntArg(height);
    kernel->addFloatArg(sc);
    kernel->addFloatArg(cx);
    kernel->addFloatArg(cy);
    kernel->addFloatArrayArg(k4, 3, MemoryAccess::READ);
    kernel->addFloatArrayArg(dk4, 3, MemoryAccess::READ);
    kernel->addRandomSeedArg();
    kernel->addBoolArg(m_jitter);
  });
}