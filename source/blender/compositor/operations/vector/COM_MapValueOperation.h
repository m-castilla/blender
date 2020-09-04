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
#include "DNA_texture_types.h"

/**
 * this program converts an input color to an output value.
 * it assumes we are in sRGB color space.
 */
class MapValueOperation : public NodeOperation {
 private:
  float m_min;
  float m_max;
  float m_size;
  float m_loc;
  int m_flag;

 public:
  /**
   * Default constructor
   */
  MapValueOperation();

  void setSettings(TexMapping *settings)
  {
    m_min = settings->min[0];
    m_max = settings->max[0];
    m_size = settings->size[0];
    m_loc = settings->loc[0];
    m_flag = settings->flag;
  }

 protected:
  void hashParams() override;
  void execPixels(ExecutionManager &man) override;
};
