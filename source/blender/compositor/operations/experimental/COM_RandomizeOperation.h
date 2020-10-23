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

class RandomizeOperation : public NodeOperation {
 protected:
  float m_min_value;
  float m_max_value;
  float m_variance_down;
  float m_variance_up;
  int m_variance_steps;
  bool m_use_fixed_seed;
  uint64_t m_fixed_seed;

  float *m_variance_table;

 public:
  RandomizeOperation();
  void setMinValue(float value)
  {
    m_min_value = value;
  }
  void setMaxValue(float value)
  {
    m_max_value = value;
  }
  void setVarianceUp(float value)
  {
    m_variance_up = value;
  }
  void setVarianceDown(float value)
  {
    m_variance_down = value;
  }
  void setVarianceSteps(int value)
  {
    m_variance_steps = value;
  }
  void setFixedSeed(uint64_t seed)
  {
    m_use_fixed_seed = true;
    m_fixed_seed = seed;
  }

 protected:
  virtual void hashParams() override;
  virtual void execPixels(ExecutionManager &man) override;
};
