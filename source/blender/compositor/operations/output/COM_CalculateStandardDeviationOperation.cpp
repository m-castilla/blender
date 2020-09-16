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

#include "COM_CalculateStandardDeviationOperation.h"
#include "BLI_math.h"
#include "BLI_utildefines.h"
#include <math.h>

#include "COM_ExecutionManager.h"
#include "IMB_colormanagement.h"

#include "COM_kernel_cpu.h"

CalculateStandardDeviationOperation::CalculateStandardDeviationOperation()
    : CalculateMeanOperation(false)
{
  addInputSocket(SocketType::COLOR);
  addInputSocket(SocketType::VALUE);
  addOutputSocket(SocketType::VALUE);
}

float *CalculateStandardDeviationOperation::getSingleElem(ExecutionManager & /*man*/)
{
  if (!m_calculated) {
    m_result = sqrt(m_sum / m_n_pixels);
    m_calculated = true;
  }
  return &m_result;
}

void CalculateStandardDeviationOperation::execPixels(ExecutionManager &man)
{
  NodeOperation *color_op = getInputOperation(0);
  NodeOperation *mean_op = getInputOperation(1);
  BLI_assert(typeid(*mean_op) == typeid(CalculateMeanOperation));
  BLI_assert(((CalculateMeanOperation *)mean_op)->getSetting() == m_setting);
  auto src_color = color_op->getPixels(this, man);
  auto src_mean = mean_op->getPixels(this, man);
  if (man.canExecPixels()) {
    BLI_assert(src_mean->is_single_elem);
    float mean = *src_mean->single_elem;
    int setting = m_setting;
    auto cpu_write = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
      float sum = 0.0f;
      int n_pixels = 0;
      float value = 0.0f;
      READ_DECL(src_color);
      WRITE_DECL(dst);
      CPU_LOOP_START(dst);
      COPY_COORDS(src_color, dst_coords);

      if (src_color_img.buffer[src_color_offset + 3] > 0.0f) {
        switch (setting) {
            // combined channels luminance sum
          case 1:
            value = IMB_colormanagement_get_luminance(&src_color_img.buffer[src_color_offset]);
            break;
            // r channel
          case 2:
            value = src_color_img.buffer[src_color_offset];
            break;
            // g channel
          case 3:
            value = src_color_img.buffer[src_color_offset + 1];
            break;
            // b channel
          case 4:
            value = src_color_img.buffer[src_color_offset + 2];
            break;
            // yuv luminance sum
          case 5:
            READ_IMG(src_color, src_color_pix);
            src_color_pix = CCL::rgb_to_yuv(src_color_pix);
            value = src_color_pix.x;
            break;
          default:
            BLI_assert(!"Non implemented setting option");
            break;
        }
        sum += (value - mean) * (value - mean);
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
}
