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
 * Copyright 2013, Blender Foundation.
 */

#include "COM_PlaneDistortCommonOperation.h"

#include "MEM_guardedalloc.h"

#include "BLI_jitter_2d.h"
#include "BLI_listbase.h"
#include "BLI_math.h"
#include "BLI_math_color.h"

#include "BKE_movieclip.h"
#include "BKE_node.h"
#include "BKE_tracking.h"
#include "COM_ExecutionManager.h"

#include "COM_kernel_cpu.h"

PlaneDistortBaseOperation::PlaneDistortBaseOperation()
{
  this->m_motion_blur_samples = 1;
  this->m_motion_blur_shutter = 0.5f;
}

void PlaneDistortBaseOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_motion_blur_samples);
  hashParam(m_motion_blur_shutter);
}

void PlaneDistortBaseOperation::saveCorners(const float corners[4][2], bool normalized, int sample)
{
  BLI_assert(sample < this->m_motion_blur_samples);
  MotionSample *sample_data = &this->m_samples[sample];
  if (normalized) {
    for (int i = 0; i < 4; i++) {
      sample_data->frameSpaceCorners[i][0] = corners[i][0] * this->getWidth();
      sample_data->frameSpaceCorners[i][1] = corners[i][1] * this->getHeight();
    }
  }
  else {
    for (int i = 0; i < 4; i++) {
      sample_data->frameSpaceCorners[i][0] = corners[i][0];
      sample_data->frameSpaceCorners[i][1] = corners[i][1];
    }
  }
}

/* ******** PlaneDistort WarpImage ******** */
BLI_INLINE void warpCoord(float x,
                          float y,
                          float matrix[3][3],
                          CCL::float2 &uv,
                          CCL::float2 &deriv1,
                          CCL::float2 &deriv2)
{
  float vec[3] = {x, y, 1.0f};
  mul_m3_v3(matrix, vec);
  uv.x = vec[0] / vec[2];
  uv.y = vec[1] / vec[2];

  deriv1.x = (matrix[0][0] - matrix[0][2] * uv.x) / vec[2];
  deriv2.x = (matrix[0][1] - matrix[0][2] * uv.y) / vec[2];
  deriv1.y = (matrix[1][0] - matrix[1][2] * uv.x) / vec[2];
  deriv2.y = (matrix[1][1] - matrix[1][2] * uv.y) / vec[2];
}

PlaneDistortWarpImageOperation::PlaneDistortWarpImageOperation() : PlaneDistortBaseOperation()
{
  this->addInputSocket(SocketType::COLOR, InputResizeMode::NO_RESIZE);
  this->addOutputSocket(SocketType::COLOR);
}

void PlaneDistortWarpImageOperation::saveCorners(const float corners[4][2],
                                                 bool normalized,
                                                 int sample)
{
  PlaneDistortBaseOperation::saveCorners(corners, normalized, sample);

  const int width = getInputOperation(0)->getWidth();
  const int height = getInputOperation(0)->getHeight();
  float frame_corners[4][2] = {
      {0.0f, 0.0f}, {(float)width, 0.0f}, {(float)width, (float)height}, {0.0f, (float)height}};

  MotionSample *sample_data = &this->m_samples[sample];
  BKE_tracking_homography_between_two_quads(
      sample_data->frameSpaceCorners, frame_corners, sample_data->perspectiveMatrix);
}

void PlaneDistortWarpImageOperation::execPixels(ExecutionManager &man)
{
  auto src = getInputOperation(0)->getPixels(this, man);
  readCorners(this, man);

  PixelsSampler nearest_sampler = PixelsSampler{PixelInterpolation::NEAREST, PixelExtend::CLIP};
  int src_w, src_h;
  float inv_w, inv_h, sqrt_w, height_by_sqrt_w;
  if (man.canExecPixels()) {
    src_w = src->getWidth();
    src_h = src->getHeight();
    inv_w = 1.0f / src_w;
    inv_h = 1.0f / src_h;
    sqrt_w = sqrtf(src_w);
    height_by_sqrt_w = src_h / sqrt_w;
  }

  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    READ_DECL(src);
    WRITE_DECL(dst);
    CCL::float2 uv;
    CCL::float2 deriv1;
    CCL::float2 deriv2;

    if (this->m_motion_blur_samples == 1) {
      auto sample = this->m_samples[0];
      CPU_LOOP_START(dst);
      warpCoord(dst_coords.x, dst_coords.y, sample.perspectiveMatrix, uv, deriv1, deriv2);
      EWA_FILTER_IMG(src,
                     nearest_sampler,
                     uv,
                     deriv1,
                     deriv2,
                     src_w,
                     src_h,
                     inv_w,
                     inv_h,
                     sqrt_w,
                     height_by_sqrt_w,
                     src_pix);
      WRITE_IMG4(dst, src_pix);
      CPU_LOOP_END;
    }
    else {
      CPU_LOOP_START(dst);

      CCL::float4 result = CCL::ZERO_F4;
      for (int sample = 0; sample < this->m_motion_blur_samples; sample++) {
        warpCoord(dst_coords.x,
                  dst_coords.y,
                  this->m_samples[sample].perspectiveMatrix,
                  uv,
                  deriv1,
                  deriv2);
        EWA_FILTER_IMG(src,
                       nearest_sampler,
                       uv,
                       deriv1,
                       deriv2,
                       src_w,
                       src_h,
                       inv_w,
                       inv_h,
                       sqrt_w,
                       height_by_sqrt_w,
                       src_pix);
        result += src_pix;
      }
      result *= 1.0f / (float)this->m_motion_blur_samples;
      WRITE_IMG4(dst, result);
      CPU_LOOP_END;
    }
  };

  return cpuWriteSeek(man, cpuWrite);
}

/* ******** PlaneDistort Mask ******** */

PlaneDistortMaskOperation::PlaneDistortMaskOperation() : PlaneDistortBaseOperation()
{
  addOutputSocket(SocketType::VALUE);

  /* Currently hardcoded to 8 samples. */
  m_osa = 8;
}

void PlaneDistortMaskOperation::initExecution()
{
  NodeOperation::initExecution();
  BLI_jitter_init(m_jitter, m_osa);
}

void PlaneDistortMaskOperation::execPixels(ExecutionManager &man)
{
  readCorners(this, man);

  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    WRITE_DECL(dst);
    CCL::float4 result;

    float point[2];
    if (this->m_motion_blur_samples == 1) {
      CPU_LOOP_START(dst);
      int inside_counter = 0;
      MotionSample *sample_data = &this->m_samples[0];
      for (int sample = 0; sample < this->m_osa; sample++) {
        point[0] = dst_coords.x + this->m_jitter[sample][0];
        point[1] = dst_coords.y + this->m_jitter[sample][1];
        if (isect_point_tri_v2(point,
                               sample_data->frameSpaceCorners[0],
                               sample_data->frameSpaceCorners[1],
                               sample_data->frameSpaceCorners[2]) ||
            isect_point_tri_v2(point,
                               sample_data->frameSpaceCorners[0],
                               sample_data->frameSpaceCorners[2],
                               sample_data->frameSpaceCorners[3])) {
          inside_counter++;
        }
      }
      result.x = (float)inside_counter / this->m_osa;
      WRITE_IMG4(dst, result);
      CPU_LOOP_END;
    }
    else {
      CPU_LOOP_START(dst);
      int inside_counter = 0;
      for (int motion_sample = 0; motion_sample < this->m_motion_blur_samples; motion_sample++) {
        MotionSample *sample_data = &this->m_samples[motion_sample];
        for (int osa_sample = 0; osa_sample < this->m_osa; osa_sample++) {
          point[0] = dst_coords.x + this->m_jitter[osa_sample][0];
          point[1] = dst_coords.y + this->m_jitter[osa_sample][1];
          if (isect_point_tri_v2(point,
                                 sample_data->frameSpaceCorners[0],
                                 sample_data->frameSpaceCorners[1],
                                 sample_data->frameSpaceCorners[2]) ||
              isect_point_tri_v2(point,
                                 sample_data->frameSpaceCorners[0],
                                 sample_data->frameSpaceCorners[2],
                                 sample_data->frameSpaceCorners[3])) {
            inside_counter++;
          }
        }
      }
      result.x = (float)inside_counter / (this->m_osa * this->m_motion_blur_samples);
      WRITE_IMG4(dst, result);
      CPU_LOOP_END;
    }
  };
  return cpuWriteSeek(man, cpuWrite);
}
