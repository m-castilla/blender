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

#include "COM_QualityStepHelper.h"
#include "COM_SimpleOperation.h"

namespace blender::compositor {

class DirectionalBlurOperation : public SimpleOperation, public QualityStepHelper {
 private:
  NodeDBlurData *m_data;
  float m_center_x_pix, m_center_y_pix;
  float m_tx, m_ty;
  float m_sc, m_rot;

 public:
  DirectionalBlurOperation();
  /**
   * Initialize the execution
   */
  void initExecution() override;

  void execPixelsMultiCPU(const rcti &render_rect,
                          CPUBuffer<float> &output,
                          blender::Span<const CPUBuffer<float> *> inputs,
                          ExecutionSystem *exec_system,
                          int current_pass) override;
  void execPixelsGPU(const rcti &render_rect,
                     GPUBuffer &output,
                     blender::Span<const GPUBuffer *> inputs,
                     ExecutionSystem *exec_system) override;

  void setData(NodeDBlurData *data)
  {
    this->m_data = data;
  }
};

}  // namespace blender::compositor
