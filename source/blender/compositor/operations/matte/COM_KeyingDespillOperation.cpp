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

#include "COM_KeyingDespillOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
KeyingDespillOperation::KeyingDespillOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);

  this->m_despillFactor = 0.5f;
  this->m_colorBalance = 0.5f;
}

void KeyingDespillOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_despillFactor);
  hashParam(m_colorBalance);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel keyingDespillOp(
    CCL_WRITE(dst), CCL_READ(color), CCL_READ(screen), float despill_factor, float color_balance)
{
  READ_DECL(color);
  READ_DECL(screen);
  WRITE_DECL(dst);
  float *color_a;

  CPU_LOOP_START(dst);

  COPY_COORDS(color, dst_coords);
  COPY_COORDS(screen, dst_coords);

  READ_IMG4(color, color_pix);
  READ_IMG4(screen, screen_pix);

  const int screen_primary_channel = max_channel_f4_3(screen_pix);
  const int other_1 = (screen_primary_channel + 1) % 3;
  const int other_2 = (screen_primary_channel + 2) % 3;

  const int min_channel = min(other_1, other_2);
  const int max_channel = max(other_1, other_2);

  color_a = (float *)&color_pix;
  float average_value, amount;
  average_value = color_balance * color_a[min_channel] +
                  (1.0f - color_balance) * color_a[max_channel];
  amount = (color_a[screen_primary_channel] - average_value);

  const float amount_despill = despill_factor * amount;
  if (amount_despill > 0.0f) {
    color_a[screen_primary_channel] = color_a[screen_primary_channel] - amount_despill;
    color_pix = make_float4(color_a[0], color_a[1], color_a[2], color_pix.w);
  }
  WRITE_IMG4(dst, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void KeyingDespillOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto screen = getInputOperation(1)->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::keyingDespillOp, _1, color, screen, m_despillFactor, m_colorBalance);
  computeWriteSeek(man, cpu_write, "keyingDespillOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*screen);
    kernel->addFloatArg(m_despillFactor);
    kernel->addFloatArg(m_colorBalance);
  });
}
