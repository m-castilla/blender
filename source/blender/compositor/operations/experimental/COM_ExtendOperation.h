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
#include "COM_Pixels.h"

class ExtendOperation : public NodeOperation {
 protected:
  float m_add_extend_w;
  float m_add_extend_h;
  float m_scale;
  PixelExtend m_extend_mode;

  int m_read_offset_x, m_read_offset_y;

 public:
  ExtendOperation();
  void setAddExtendW(float w)
  {
    this->m_add_extend_w = w;
  }
  void setAddExtendH(float h)
  {
    this->m_add_extend_h = h;
  }
  void setExtendMode(PixelExtend extend_mode)
  {
    this->m_extend_mode = extend_mode;
  }
  void setScale(float scale)
  {
    m_scale = scale;
  }
  ResolutionType determineResolution(int resolution[2],
                                     int preferredResolution[2],
                                     bool setResolution) override;

 protected:
  virtual void hashParams() override;
  virtual void execPixels(ExecutionManager &man) override;
};
