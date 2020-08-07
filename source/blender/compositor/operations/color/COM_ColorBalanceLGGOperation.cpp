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
ColorBalanceLGGOperation::ColorBalanceLGGOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
  this->setMainInputSocketIndex(1);
}
void ColorBalanceLGGOperation::hashParams()
{
  NodeOperation::hashParams();
  hashDataAsParam(m_gain, 3);
  hashDataAsParam(m_lift, 3);
  hashDataAsParam(m_gamma_inv, 3);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel colorBalanceLGGOp(
    CCL_WRITE(dst), CCL_READ(value), CCL_READ(color), float4 lift, float4 gamma_inv, float4 gain)
{
  READ_DECL(value);
  READ_DECL(color);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(value, dst_coords, value_pix);
  READ_IMG(color, dst_coords, color_pix);

  const float fac = fminf(1.0f, value_pix.x);
  const float mfac = 1.0f - fac;

  float4 res = mfac * color_pix + fac * colorbalance_lgg(color_pix, lift, gamma_inv, gain);
  res.w = color_pix.w;

  WRITE_IMG(dst, dst_coords, res);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void ColorBalanceLGGOperation::execPixels(ExecutionManager &man)
{
  auto value = getInputOperation(0)->getPixels(this, man);
  auto color = getInputOperation(1)->getPixels(this, man);
  auto lift = CCL_NAMESPACE::make_float4(m_lift[0], m_lift[1], m_lift[2], 1.0f);
  auto gamma_inv = CCL_NAMESPACE::make_float4(
      m_gamma_inv[0], m_gamma_inv[1], m_gamma_inv[2], 1.0f);
  auto gain = CCL_NAMESPACE::make_float4(m_gain[0], m_gain[1], m_gain[2], 1.0f);
  auto cpu_write = std::bind(
      CCL_NAMESPACE::colorBalanceLGGOp, _1, value, color, lift, gamma_inv, gain);
  computeWriteSeek(man, cpu_write, "colorBalanceLGGOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color);
    kernel->addFloat4Arg((float *)&lift);
    kernel->addFloat4Arg((float *)&gamma_inv);
    kernel->addFloat4Arg((float *)&gain);
  });
}
