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

#include "COM_SetColorOperation.h"

namespace blender::compositor {

SetColorOperation::SetColorOperation()
{
  this->addOutputSocket(DataType::Color);
  flags.is_set_operation = true;
}

void SetColorOperation::execPixelsCPU(const rcti &render_rect,
                                      CPUBuffer<float> &output,
                                      blender::Span<const CPUBuffer<float> *> inputs,
                                      ExecutionSystem *exec_system)
{
  BLI_assert(output.is_single_elem);
  copy_v4_v4(&output[0], this->m_color);
}

void SetColorOperation::determineResolution(unsigned int resolution[2],
                                            unsigned int preferredResolution[2])
{
  resolution[0] = preferredResolution[0];
  resolution[1] = preferredResolution[1];
}

}  // namespace blender::compositor
