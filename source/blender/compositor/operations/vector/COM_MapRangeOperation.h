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
 * Copyright 2012, Blender Foundation.
 */

#pragma once

#include "COM_NodeOperation.h"

/**
 * this program converts an input color to an output value.
 * it assumes we are in sRGB color space.
 */
class MapRangeOperation : public NodeOperation {
 private:
  bool m_use_clamp;

 public:
  /**
   * Default constructor
   */
  MapRangeOperation();

  /**
   * Clamp the output
   */
  void setUseClamp(bool value)
  {
    this->m_use_clamp = value;
  }

 protected:
  void hashParams() override;
  void execPixels(ExecutionManager &man) override;
};
