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
class SetValueOperation : public NodeOperation {
 private:
  float m_value[1];

 public:
  /**
   * Default constructor
   */
  SetValueOperation();

  virtual bool canCompute() const override
  {
    return false;
  }

  float getValue()
  {
    return *m_value;
  }
  void setValue(float value)
  {
    *m_value = value;
  }

  BufferType getBufferType() const override
  {
    return BufferType::NO_BUFFER_NO_WRITE;
  }
  bool isSingleElem() const override
  {
    return true;
  }
  float *getSingleElem() override
  {
    return m_value;
  }

 protected:
  void hashParams() override;
};
