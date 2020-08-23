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

#include "COM_GlareThresholdOperation.h"
#include "COM_ExecutionManager.h"
#include "COM_GlobalManager.h"
#include "COM_PixelsUtil.h"
#include "IMB_colormanagement.h"

#include "COM_kernel_cpu.h"

GlareThresholdOperation::GlareThresholdOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR, InputResizeMode::FIT);
  this->addOutputSocket(SocketType::COLOR);
}

void GlareThresholdOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_settings->quality);
  hashParam(m_settings->threshold);
}

ResolutionType GlareThresholdOperation::determineResolution(int resolution[2],
                                                            int preferredResolution[2],
                                                            bool setResolution)
{
  NodeOperation::determineResolution(resolution, preferredResolution, setResolution);
  resolution[0] = resolution[0] / (1 << this->m_settings->quality);
  resolution[1] = resolution[1] / (1 << this->m_settings->quality);
  return ResolutionType::Determined;
}

void GlareThresholdOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  const float threshold = this->m_settings->threshold;
  auto cpu_write = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    READ_DECL(color);
    WRITE_DECL(dst);

    CPU_LOOP_START(dst);

    COPY_COORDS(color, dst_coords);

    dst_img.buffer[dst_offset] = color_img.buffer[color_offset];
    dst_img.buffer[dst_offset + 1] = color_img.buffer[color_offset + 1];
    dst_img.buffer[dst_offset + 2] = color_img.buffer[color_offset + 2];
    dst_img.buffer[dst_offset + 3] = color_img.buffer[color_offset + 3];

    if (IMB_colormanagement_get_luminance(&dst_img.buffer[dst_offset]) >= threshold) {
      dst_img.buffer[dst_offset] -= threshold;
      dst_img.buffer[dst_offset + 1] -= threshold;
      dst_img.buffer[dst_offset + 2] -= threshold;

      dst_img.buffer[dst_offset] = fmaxf(dst_img.buffer[dst_offset], 0.0f);
      dst_img.buffer[dst_offset + 1] = fmaxf(dst_img.buffer[dst_offset + 1], 0.0f);
      dst_img.buffer[dst_offset + 2] = fmaxf(dst_img.buffer[dst_offset + 2], 0.0f);
    }
    else {
      dst_img.buffer[dst_offset] = 0.0f;
      dst_img.buffer[dst_offset + 1] = 0.0f;
      dst_img.buffer[dst_offset + 2] = 0.0f;
    }

    CPU_LOOP_END
  };
  cpuWriteSeek(man, cpu_write);
}
