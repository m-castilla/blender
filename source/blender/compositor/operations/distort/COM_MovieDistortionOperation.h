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

#pragma once

#include "COM_NodeOperation.h"
#include "DNA_movieclip_types.h"
#include "MEM_guardedalloc.h"

#include "BKE_tracking.h"

class MovieDistortionOperation : public NodeOperation {
 private:
  MovieClip *m_movieClip;

 protected:
  bool m_apply;
  int m_framenumber;

  struct MovieDistortion *m_distortion;
  int m_calibration_width, m_calibration_height;
  float m_pixel_aspect;

 public:
  MovieDistortionOperation(bool distortion);

  void initExecution();
  void deinitExecution();

  void setMovieClip(MovieClip *clip)
  {
    this->m_movieClip = clip;
  }
  void setFramenumber(int framenumber)
  {
    this->m_framenumber = framenumber;
  }

 protected:
  bool canCompute() const override
  {
    return false;
  }
  void execPixels(ExecutionManager &man) override;
  void hashParams() override;
};
