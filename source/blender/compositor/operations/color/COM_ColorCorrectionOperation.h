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

class ColorCorrectionOperation : public NodeOperation {
 private:
  NodeColorCorrection *m_data;

  bool m_redChannelEnabled;
  bool m_greenChannelEnabled;
  bool m_blueChannelEnabled;

 public:
  ColorCorrectionOperation();

  void setData(NodeColorCorrection *data)
  {
    this->m_data = data;
  }
  void setRedChannelEnabled(bool enabled)
  {
    this->m_redChannelEnabled = enabled;
  }
  void setGreenChannelEnabled(bool enabled)
  {
    this->m_greenChannelEnabled = enabled;
  }
  void setBlueChannelEnabled(bool enabled)
  {
    this->m_blueChannelEnabled = enabled;
  }

 protected:
  virtual bool canCompute() const override
  {
    return false;
  }
  virtual void hashParams();
  virtual void execPixels(ExecutionManager &man) override;
};
