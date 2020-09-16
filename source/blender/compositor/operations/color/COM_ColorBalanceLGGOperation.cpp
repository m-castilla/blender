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

#include "COM_ColorBalanceLGGOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_Pixels.h"
#include "COM_Rect.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
ColorBalanceLGGOperation::ColorBalanceLGGOperation()
    : NodeOperation(), m_gain(), m_lift(), m_gamma_inv()
{
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
  this->setMainInputSocketIndex(1);
}
void ColorBalanceLGGOperation::hashParams()
{
  NodeOperation::hashParams();
  hashFloatData(m_gain, 3);
  hashFloatData(m_lift, 3);
  hashFloatData(m_gamma_inv, 3);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_device_inline float4 colorbalance_lgg(float4 in,
                                          float4 lift_lgg,
                                          float4 gamma_inv,
                                          float4 gain)
{
  /* 1:1 match with the sequencer with linear/srgb conversions, the conversion isnt pretty
   * but best keep it this way, since testing for durian shows a similar calculation
   * without lin/srgb conversions gives bad results (over-saturated shadows) with colors
   * slightly below 1.0. some correction can be done but it ends up looking bad for shadows or
   * lighter tones - campbell */
  float4 res;
  res.x = linearrgb_to_srgb(in.x);
  res.y = linearrgb_to_srgb(in.y);
  res.z = linearrgb_to_srgb(in.z);
  res = (((res - 1.0f) * lift_lgg) + 1.0f) * gain;
  if (res.x < 0.0f) {
    res.x = 0.0f;
  }
  if (res.y < 0.0f) {
    res.y = 0.0f;
  }
  if (res.z < 0.0f) {
    res.z = 0.0f;
  }
  res.x = powf(srgb_to_linearrgb(res.x), gamma_inv.x);
  res.y = powf(srgb_to_linearrgb(res.y), gamma_inv.y);
  res.z = powf(srgb_to_linearrgb(res.z), gamma_inv.z);
  res.w = in.w;

  return res;
}

ccl_kernel colorBalanceLGGOp(
    CCL_WRITE(dst), CCL_READ(value), CCL_READ(color), float4 lift, float4 gamma_inv, float4 gain)
{
  READ_DECL(value);
  READ_DECL(color);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color, color_pix);

  const float fac = fminf(1.0f, value_pix.x);
  const float mfac = 1.0f - fac;

  float4 res = mfac * color_pix + fac * colorbalance_lgg(color_pix, lift, gamma_inv, gain);
  res.w = color_pix.w;

  WRITE_IMG4(dst, res);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void ColorBalanceLGGOperation::execPixels(ExecutionManager &man)
{
  auto value = getInputOperation(0)->getPixels(this, man);
  auto color = getInputOperation(1)->getPixels(this, man);
  auto lift = CCL::make_float4(m_lift[0], m_lift[1], m_lift[2], 1.0f);
  auto gamma_inv = CCL::make_float4(m_gamma_inv[0], m_gamma_inv[1], m_gamma_inv[2], 1.0f);
  auto gain = CCL::make_float4(m_gain[0], m_gain[1], m_gain[2], 1.0f);
  auto cpu_write = std::bind(CCL::colorBalanceLGGOp, _1, value, color, lift, gamma_inv, gain);
  computeWriteSeek(man, cpu_write, "colorBalanceLGGOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color);
    kernel->addFloat4Arg(lift);
    kernel->addFloat4Arg(gamma_inv);
    kernel->addFloat4Arg(gain);
  });
}
