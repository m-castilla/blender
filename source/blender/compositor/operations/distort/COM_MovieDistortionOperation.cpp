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

#include "COM_MovieDistortionOperation.h"

#include "BKE_movieclip.h"
#include "BKE_tracking.h"
#include "BLI_linklist.h"

#include "COM_kernel_cpu.h"

MovieDistortionOperation::MovieDistortionOperation(bool distortion) : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
  this->m_movieClip = NULL;
  this->m_apply = distortion;
}

void MovieDistortionOperation::hashParams()
{
  NodeOperation::hashParams();

  hashParam(m_framenumber);
  hashParam(m_apply);
  if (m_movieClip) {
    hashParam(m_movieClip->id.session_uuid);
    hashParam(m_calibration_width);
    hashParam(m_calibration_height);
    hashParam(m_pixel_aspect);

    const MovieTrackingCamera *camera = &m_movieClip->tracking.camera;
    hashFloatData(camera->principal, 2);
    hashParam(camera->focal);
    hashParam(camera->units);
    hashParam(camera->sensor_width);
    hashParam(camera->distortion_model);
    hashParam(camera->division_k1);
    hashParam(camera->division_k2);
    hashParam(camera->k1);
    hashParam(camera->k2);
    hashParam(camera->k3);
    hashParam(camera->nuke_k1);
    hashParam(camera->nuke_k2);
  }
}

void MovieDistortionOperation::initExecution()
{
  if (this->m_movieClip) {
    MovieTracking *tracking = &this->m_movieClip->tracking;
    MovieClipUser clipUser = {0};
    int calibration_width, calibration_height;

    BKE_movieclip_user_set_frame(&clipUser, this->m_framenumber);
    BKE_movieclip_get_size(this->m_movieClip, &clipUser, &calibration_width, &calibration_height);

    this->m_distortion = BKE_tracking_distortion_new(
        tracking, calibration_width, calibration_height);
    this->m_calibration_width = calibration_width;
    this->m_calibration_height = calibration_height;
    this->m_pixel_aspect = tracking->camera.pixel_aspect;
  }
  else {
    this->m_distortion = NULL;
  }
}

void MovieDistortionOperation::deinitExecution()
{
  this->m_movieClip = NULL;
  if (this->m_distortion != NULL) {
    BKE_tracking_distortion_free(this->m_distortion);
  }
}

void MovieDistortionOperation::execPixels(ExecutionManager &man)
{
  auto src = getInputOperation(0)->getPixels(this, man);

  /* float overscan = 0.0f; */
  const float pixel_aspect = this->m_pixel_aspect;
  const float w = (float)this->m_width /* / (1 + overscan) */;
  const float h = (float)this->m_height /* / (1 + overscan) */;
  const float aspx = w / (float)this->m_calibration_width;
  const float aspy = h / (float)this->m_calibration_height;
  PixelsSampler sampler = PixelsSampler{PixelInterpolation::BILINEAR, PixelExtend::CLIP};
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    float in[2];
    float out[2];

    READ_DECL(src);
    WRITE_DECL(dst);
    CPU_LOOP_START(dst);

    if (this->m_distortion != NULL) {
      in[0] = (dst_coords.x /* - 0.5 * overscan * w */) / aspx;
      in[1] = (dst_coords.y /* - 0.5 * overscan * h */) / aspy / pixel_aspect;

      if (this->m_apply) {
        BKE_tracking_distortion_undistort_v2(this->m_distortion, in, out);
      }
      else {
        BKE_tracking_distortion_distort_v2(this->m_distortion, in, out);
      }

      float u = out[0] * aspx /* + 0.5 * overscan * w */,
            v = (out[1] * aspy /* + 0.5 * overscan * h */) * pixel_aspect;

      SET_SAMPLE_COORDS(src, u, v);
    }
    else {
      SET_SAMPLE_COORDS(src, dst_coords.x, dst_coords.y);
    }
    SAMPLE_BILINEAR4_CLIP(0, src, sampler, src_pix);
    WRITE_IMG4(dst, src_pix);

    CPU_LOOP_END;
  };
  return cpuWriteSeek(man, cpuWrite);
}
