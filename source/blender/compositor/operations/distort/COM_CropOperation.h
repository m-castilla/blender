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

#include "BLI_rect.h"
#include "COM_NodeOperation.h"

class CropBaseOperation : public NodeOperation {
 protected:
  NodeTwoXYs *m_settings;
  bool m_relative;

 public:
  CropBaseOperation();
  void setCropSettings(NodeTwoXYs *settings)
  {
    this->m_settings = settings;
  }
  void setRelative(bool rel)
  {
    this->m_relative = rel;
  }

 protected:
  rcti getCropRectForInput(int input_w, int input_h);
  virtual void hashParams() override;
};

class CropOperation : public CropBaseOperation {
 public:
  CropOperation();

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class CropImageOperation : public CropBaseOperation {
 public:
  CropImageOperation();
  virtual ResolutionType determineResolution(int resolution[2],
                                             int preferredResolution[2],
                                             bool setResolution) override;

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
