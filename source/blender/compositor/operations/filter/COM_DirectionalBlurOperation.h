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
#include "COM_QualityStepHelper.h"

class DirectionalBlurOperation : public NodeOperation, public QualityStepHelper {
 private:
  NodeDBlurData *m_data;

  float m_pix_center_x, m_pix_center_y;
  float m_tx, m_ty;
  float m_sc, m_rot;
  int m_iterations;

 public:
  DirectionalBlurOperation();

  void initExecution();

  void setData(NodeDBlurData *data)
  {
    this->m_data = data;
  }

 protected:
  virtual void hashParams() override;
  virtual void execPixels(ExecutionManager &man) override;
};
