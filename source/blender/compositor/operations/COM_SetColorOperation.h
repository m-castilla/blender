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

/**
 * this program converts an input color to an output value.
 * it assumes we are in sRGB color space.
 */
class SetColorOperation : public NodeOperation {
 private:
  float m_color[4];

 public:
  /**
   * Default constructor
   */
  SetColorOperation();

  virtual bool canCompute() const override
  {
    return false;
  }

  float getChannel0()
  {
    return m_color[0];
  }
  void setChannel0(float value)
  {
    m_color[0] = value;
  }
  float getChannel1()
  {
    return m_color[1];
  }
  void setChannel1(float value)
  {
    m_color[1] = value;
  }
  float getChannel2()
  {
    return m_color[2];
  }
  void setChannel2(float value)
  {
    m_color[2] = value;
  }
  float getChannel3()
  {
    return m_color[3];
  }
  void setChannel3(float value)
  {
    m_color[3] = value;
  }
  void setChannels(const float value[COM_NUM_CHANNELS_COLOR]);
  BufferType getBufferType() const override
  {
    return BufferType::NO_BUFFER_NO_WRITE;
  }
  bool isSingleElem() const override
  {
    return true;
  }
  float *getSingleElem(ExecutionManager & /*man*/) override
  {
    return m_color;
  }
  ResolutionType determineResolution(int resolution[2],
                                     int preferredResolution[2],
                                     bool setResolution) override;

 protected:
  void hashParams() override;
};
