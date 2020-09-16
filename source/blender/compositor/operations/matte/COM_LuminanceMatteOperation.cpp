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
#include "IMB_colormanagement.h"

#include "COM_LuminanceMatteOperation.h"

#include "COM_kernel_cpu.h"

LuminanceMatteOperation::LuminanceMatteOperation() : NodeOperation()
{
  addInputSocket(SocketType::COLOR);
  addOutputSocket(SocketType::VALUE);
}

void LuminanceMatteOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_settings->t1);
  hashParam(m_settings->t2);
}

void LuminanceMatteOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  const float high = m_settings->t1;
  const float low = m_settings->t2;

  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    READ_DECL(color);
    WRITE_DECL(dst);

    float alpha;

    CPU_LOOP_START(dst);

    COPY_COORDS(color, dst_coords);

    READ_IMG4(color, color_pix);

    const float luminance = IMB_colormanagement_get_luminance((float *)&color_pix);

    /* one line thread-friend algorithm:
     * output[0] = min(inputValue[3], min(1.0f, max(0.0f, ((luminance - low) / (high - low))));
     */

    /* test range */
    if (luminance > high) {
      alpha = 1.0f;
    }
    else if (luminance < low) {
      alpha = 0.0f;
    }
    else { /*blend */
      alpha = (luminance - low) / (high - low);
    }

    /* store matte(alpha) value in [0] to go with
     * COM_SetAlphaOperation and the Value output
     */

    /* don't make something that was more transparent less transparent */
    color_pix.x = fminf(alpha, color_pix.w);

    WRITE_IMG1(dst, color_pix);

    CPU_LOOP_END;
  };
  return cpuWriteSeek(man, cpuWrite);
}
