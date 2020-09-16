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

#include "COM_MovieClipAttributeOperation.h"

#include "BKE_movieclip.h"
#include "BKE_tracking.h"

#include "COM_ComputeKernel.h"

#include "COM_kernel_cpu.h"

using namespace std::placeholders;
MovieClipAttributeOperation::MovieClipAttributeOperation() : NodeOperation()
{
  this->addOutputSocket(SocketType::VALUE);
  this->m_framenumber = 0;
  this->m_attribute = MovieClipAttribute::MCA_X;
  this->m_invert = false;
  m_value = 0.0f;
  m_clip = nullptr;
}

void MovieClipAttributeOperation::hashParams()
{
  NodeOperation::hashParams();

  hashParam(m_framenumber);
  hashParam(m_attribute);
  hashParam(m_invert);
  hashParam(m_value);
  if (m_clip) {
    hashParam(m_clip->id.session_uuid);
  }
}

void MovieClipAttributeOperation::initExecution()
{
  if (!m_initialized) {
    NodeOperation::initExecution();
    if (!this->m_clip) {
      return;
    }
    float loc[2], scale, angle;
    loc[0] = 0.0f;
    loc[1] = 0.0f;
    scale = 1.0f;
    angle = 0.0f;
    int clip_framenr = BKE_movieclip_remap_scene_to_clip_frame(this->m_clip, this->m_framenumber);
    BKE_tracking_stabilization_data_get(
        this->m_clip, clip_framenr, getWidth(), getHeight(), loc, &scale, &angle);
    switch (this->m_attribute) {
      case MovieClipAttribute::MCA_SCALE:
        this->m_value = scale;
        break;
      case MovieClipAttribute::MCA_ANGLE:
        this->m_value = angle;
        break;
      case MovieClipAttribute::MCA_X:
        this->m_value = loc[0];
        break;
      case MovieClipAttribute::MCA_Y:
        this->m_value = loc[1];
        break;
    }
    if (this->m_invert) {
      if (this->m_attribute != MovieClipAttribute::MCA_SCALE) {
        this->m_value = -this->m_value;
      }
      else {
        this->m_value = 1.0f / this->m_value;
      }
    }
  }
}

float *MovieClipAttributeOperation::getSingleElem(ExecutionManager &man)
{
  initExecution();  // assure initialized
  return &m_value;
}