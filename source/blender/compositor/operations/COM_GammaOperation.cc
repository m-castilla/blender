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

#include "COM_GammaOperation.h"
#include "BLI_math.h"

namespace blender::compositor {

GammaOperation::GammaOperation()
{
  this->addInputSocket(DataType::Color);
  this->addInputSocket(DataType::Value);
  this->addOutputSocket(DataType::Color);
}

void GammaOperation::execPixelsMultiCPU(const rcti &render_rect,
                                        CPUBuffer<float> &output,
                                        blender::Span<const CPUBuffer<float> *> inputs,
                                        ExecutionSystem *exec_system,
                                        int current_pass)
{
  auto &input = *inputs[0];
  auto &gamma = *inputs[1];
  for (int y = render_rect.ymin; y < render_rect.ymax; y++) {
    int x = render_rect.xmin;
    float *output_elem = output.getElem(x, y);
    const float *input_elem = input.getElem(x, y);
    const float *gamma_elem = gamma.getElem(x, y);
    for (; x < render_rect.xmax; x++) {
      const float gamma_value = gamma_elem[0];
      /* check for negative to avoid nan's */
      output_elem[0] = input_elem[0] > 0.0f ? powf(input_elem[0], gamma_value) : input_elem[0];
      output_elem[1] = input_elem[1] > 0.0f ? powf(input_elem[1], gamma_value) : input_elem[1];
      output_elem[2] = input_elem[2] > 0.0f ? powf(input_elem[2], gamma_value) : input_elem[2];
      output_elem[3] = input_elem[3];

      output_elem += output.elem_jump;
      input_elem += input.elem_jump;
      gamma_elem += gamma.elem_jump;
    }
  }
}

}  // namespace blender::compositor
