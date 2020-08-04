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

class SplitOperation : public NodeOperation {
 private:
  float m_splitPercentage;
  bool m_xSplit;

 public:
  SplitOperation();
  // void determineResolution(int resolution[2], int preferredResolution[2]) override;
  void setSplitPercentage(float splitPercentage)
  {
    this->m_splitPercentage = splitPercentage;
  }
  void setXSplit(bool xsplit)
  {
    this->m_xSplit = xsplit;
  }
  bool canCompute() const override
  {
    return false;
  }

 protected:
  void execPixels(ExecutionManager &man) override;
};
