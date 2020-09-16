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
class ColorSpillOperation : public NodeOperation {
 protected:
  NodeColorspill *m_settings;
  int m_spill_channel;
  int m_spill_method;

  /* After init set */
  int m_channel2;
  int m_channel3;
  float m_rmut, m_gmut, m_bmut;
  /* */

 public:
  /**
   * Default constructor
   */
  ColorSpillOperation();

  void initExecution();

  void setSettings(NodeColorspill *nodeColorSpill)
  {
    this->m_settings = nodeColorSpill;
  }
  void setSpillChannel(int channel)
  {
    this->m_spill_channel = channel;
  }
  void setSpillMethod(int method)
  {
    this->m_spill_method = method;
  }

 protected:
  virtual void hashParams() override;
  virtual void execPixels(ExecutionManager &man) override;
};
