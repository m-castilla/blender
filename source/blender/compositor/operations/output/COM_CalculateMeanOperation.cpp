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

#include "COM_CalculateMeanOperation.h"
#include "BLI_math.h"
#include "BLI_utildefines.h"

#include "COM_kernel_cpu_nocompat.h"
#include "IMB_colormanagement.h"

CalculateMeanOperation::CalculateMeanOperation(bool add_sockets) : NodeOperation()
{
  if (add_sockets) {
    this->addInputSocket(SocketType::COLOR, InputResizeMode::NO_RESIZE);
    this->addOutputSocket(SocketType::VALUE);
  }
  m_setting = 1;
  m_sum = 0;
  m_n_pixels = 0;
  m_result = 0;
  m_calculated = false;
}
void CalculateMeanOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_setting);
}

void CalculateMeanOperation::execPixels(ExecutionManager &man)
{
  auto src = getInputOperation(0)->getPixels(this, man);
  int setting = m_setting;
  auto cpu_write = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    float sum = 0;
    int n_pixels = 0;
    CPU_READ_DECL(src);
    CCL_NAMESPACE::float4 src_pix;
    CPU_WRITE_DECL(dst);
    CPU_LOOP_START(dst);
    CPU_READ_OFFSET(src, dst);
    if (src_img.buffer[src_offset + 3] > 0.0f) {
      switch (setting) {
          // combined channels luminance
        case 1:
          sum += IMB_colormanagement_get_luminance(&src_img.buffer[src_offset]);
          break;
          // r channel
        case 2:
          sum += src_img.buffer[src_offset];
          break;
          // g channel
        case 3:
          sum += src_img.buffer[src_offset + 1];
          break;
          // b channel
        case 4:
          sum += src_img.buffer[src_offset + 2];
          break;
          // yuv luminance sum
        case 5:
          CPU_READ_IMG(src, src_offset, src_pix);
          src_pix = CCL_NAMESPACE::rgb_to_yuv(src_pix);
          sum += src_pix.x;
          break;
        default:
          BLI_assert(!"Non implemented setting option");
          break;
      }
      n_pixels++;
    }
    CPU_LOOP_END;

    mutex.lock();
    m_sum += sum;
    m_n_pixels += n_pixels;
    mutex.unlock();
  };
  cpuWriteSeek(man, cpu_write);
}
