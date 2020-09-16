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
 * Copyright 2012, Blender Foundation.
 */

#include "COM_KeyingOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
KeyingOperation::KeyingOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::VALUE);

  this->m_screenBalance = 0.5f;
}

void KeyingOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_screenBalance);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_device_inline float get_pixel_saturation(const float rgb[3],
                                             float screen_balance,
                                             int primary_channel)
{
  const int other_1 = (primary_channel + 1) % 3;
  const int other_2 = (primary_channel + 2) % 3;

  const int min_channel = min(other_1, other_2);
  const int max_channel = max(other_1, other_2);

  const float val = screen_balance * rgb[min_channel] + (1.0f - screen_balance) * rgb[max_channel];

  return (rgb[primary_channel] - val) * fabsf(1.0f - val);
}

ccl_kernel keyingOp(CCL_WRITE(dst), CCL_READ(color), CCL_READ(screen), float screen_balance)
{
  READ_DECL(color);
  READ_DECL(screen);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(color, dst_coords);
  COPY_COORDS(screen, dst_coords);

  READ_IMG4(color, color_pix);
  READ_IMG4(screen, screen_pix);

  const int primary_channel = max_channel_f4_3(screen_pix);
  const float min_pixel_color = fminf3(color_pix.x, color_pix.y, color_pix.z);

  if (min_pixel_color > 1.0f) {
    /* overexposure doesn't happen on screen itself and usually happens
     * on light sources in the shot, this need to be checked separately
     * because saturation and falloff calculation is based on the fact
     * that pixels are not overexposed
     */
    color_pix.x = 1.0f;
  }
  else {
    float *color_a = (float *)&color_pix;
    float saturation = get_pixel_saturation(color_a, screen_balance, primary_channel);
    if (saturation < 0) {
      /* means main channel of pixel is different from screen,
       * assume this is completely a foreground
       */
      color_pix.x = 1.0f;
    }
    else {
      float *screen_a = (float *)&screen_pix;
      float screen_saturation = get_pixel_saturation(screen_a, screen_balance, primary_channel);
      if (saturation >= screen_saturation) {
        /* matched main channels and higher saturation on pixel
         * is treated as completely background
         */
        color_pix.x = 0.0f;
      }
      else {
        /* nice alpha falloff on edges */
        float distance = 1.0f - saturation / screen_saturation;

        color_pix.x = distance;
      }
    }
  }

  WRITE_IMG4(dst, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void KeyingOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto screen = getInputOperation(1)->getPixels(this, man);
  auto cpu_write = std::bind(CCL::keyingOp, _1, color, screen, m_screenBalance);
  computeWriteSeek(man, cpu_write, "keyingOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*screen);
    kernel->addFloatArg(m_screenBalance);
  });
}