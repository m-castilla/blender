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

#ifndef __COM_BOKEHBLUROPERATION_H__
#define __COM_BOKEHBLUROPERATION_H__

#include "COM_NodeOperation.h"
#include "COM_QualityStepHelper.h"

class BokehBlurOperation : public NodeOperation, public QualityStepHelper {
 private:
  float m_size;
  bool m_extend_bounds;

 public:
  BokehBlurOperation();

  void initExecution();

  void setSize(float size)
  {
    this->m_size = size;
  }

  void setExtendBounds(bool extend_bounds)
  {
    this->m_extend_bounds = extend_bounds;
  }

  ResolutionType determineResolution(int resolution[2],
                                     int preferredResolution[2],
                                     bool setResolution);

 protected:
  virtual void hashParams() override;
  virtual void execPixels(ExecutionManager &man) override;
};
#endif
