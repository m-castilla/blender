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

#pragma once

#include <string.h>

#include "COM_NodeOperation.h"

#include "DNA_movieclip_types.h"
#include "DNA_tracking_types.h"

#include "BLI_listbase.h"
#include "BLI_string.h"

#define PLANE_DISTORT_MAX_SAMPLES 64

class PlaneDistortBaseOperation : public NodeOperation {
 private:
  friend class PlaneTrackCommon;

 protected:
  struct MotionSample {
    float frameSpaceCorners[4][2]; /* Corners coordinates in pixel space. */
    float perspectiveMatrix[3][3];
  };
  MotionSample m_samples[PLANE_DISTORT_MAX_SAMPLES];
  int m_motion_blur_samples;
  float m_motion_blur_shutter;

 public:
  PlaneDistortBaseOperation();

  virtual void saveCorners(const float corners[4][2], bool normalized, int sample);

  void setMotionBlurSamples(int samples)
  {
    BLI_assert(samples <= PLANE_DISTORT_MAX_SAMPLES);
    this->m_motion_blur_samples = samples;
  }
  void setMotionBlurShutter(float shutter)
  {
    this->m_motion_blur_shutter = shutter;
  }

  virtual void readCorners(NodeOperation * /*reader_op*/, ExecutionManager & /*man*/){};

 protected:
  virtual bool canCompute() const override
  {
    return false;
  }
  virtual void hashParams() override;
};

class PlaneDistortWarpImageOperation : public PlaneDistortBaseOperation {
 public:
  PlaneDistortWarpImageOperation();

  void saveCorners(const float corners[4][2], bool normalized, int sample);

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class PlaneDistortMaskOperation : public PlaneDistortBaseOperation {
 protected:
  int m_osa;
  float m_jitter[32][2];

 public:
  PlaneDistortMaskOperation();

  virtual void initExecution() override;

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
