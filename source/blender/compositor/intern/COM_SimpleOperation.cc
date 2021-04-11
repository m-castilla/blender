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

#include "COM_SimpleOperation.h"

namespace blender::compositor {

void SimpleOperation::execPixelsCPU(const rcti &render_rect,
                                    CPUBuffer<float> &output,
                                    blender::Span<const CPUBuffer<float> *> inputs,
                                    ExecutionSystem *exec_system)
{
  for (int pass = 0; pass < getNPasses(); pass++) {
    exec_system->execWorkCPU(render_rect, [&,pass](const rcti &split_rect) {
      execPixelsMultiCPU(split_rect, output, inputs, exec_system, pass);
    });
  }
}

}  // namespace blender::compositor
