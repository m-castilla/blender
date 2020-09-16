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
class SetVectorOperation : public NodeOperation {
 private:
  float m_vector[3];

 public:
  /**
   * Default constructor
   */
  SetVectorOperation();

  virtual bool canCompute() const override
  {
    return false;
  }

  float getX()
  {
    return m_vector[0];
  }
  void setX(float value)
  {
    m_vector[0] = value;
  }
  float getY()
  {
    return m_vector[1];
  }
  void setY(float value)
  {
    m_vector[1] = value;
  }
  float getZ()
  {
    return m_vector[2];
  }
  void setZ(float value)
  {
    m_vector[2] = value;
  }

  void setVector(const float vector[3])
  {
    setX(vector[0]);
    setY(vector[1]);
    setZ(vector[2]);
  }

  bool isSingleElem() const override
  {
    return true;
  }
  float *getSingleElem(ExecutionManager &man) override
  {
    return m_vector;
  }
  BufferType getBufferType() const override
  {
    return BufferType::NO_BUFFER_NO_WRITE;
  }

 protected:
  void hashParams() override;
};
