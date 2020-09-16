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

#include "COM_ColorSpillOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;

ColorSpillOperation::ColorSpillOperation() : NodeOperation()
{
  addInputSocket(SocketType::COLOR);
  addInputSocket(SocketType::VALUE);
  addOutputSocket(SocketType::COLOR);

  this->m_spill_channel = 1;  // GREEN
  this->m_spill_method = 0;
}

void ColorSpillOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_spill_channel);
  hashParam(m_spill_method);
  hashParam(m_settings->limchan);
  hashParam(m_settings->limscale);
  hashParam(m_settings->unspill);
  hashParam(m_settings->uspillb);
  hashParam(m_settings->uspillr);
  hashParam(m_settings->uspillg);
}

void ColorSpillOperation::initExecution()
{
  NodeOperation::initExecution();
  if (this->m_spill_channel == 0) {
    this->m_rmut = -1.0f;
    this->m_gmut = 1.0f;
    this->m_bmut = 1.0f;
    this->m_channel2 = 1;
    this->m_channel3 = 2;
    if (this->m_settings->unspill == 0) {
      this->m_settings->uspillr = 1.0f;
      this->m_settings->uspillg = 0.0f;
      this->m_settings->uspillb = 0.0f;
    }
  }
  else if (this->m_spill_channel == 1) {
    this->m_rmut = 1.0f;
    this->m_gmut = -1.0f;
    this->m_bmut = 1.0f;
    this->m_channel2 = 0;
    this->m_channel3 = 2;
    if (this->m_settings->unspill == 0) {
      this->m_settings->uspillr = 0.0f;
      this->m_settings->uspillg = 1.0f;
      this->m_settings->uspillb = 0.0f;
    }
  }
  else {
    this->m_rmut = 1.0f;
    this->m_gmut = 1.0f;
    this->m_bmut = -1.0f;

    this->m_channel2 = 0;
    this->m_channel3 = 1;
    if (this->m_settings->unspill == 0) {
      this->m_settings->uspillr = 0.0f;
      this->m_settings->uspillg = 0.0f;
      this->m_settings->uspillb = 1.0f;
    }
  }
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

#define AVG(a, b) ((a + b) / 2)

ccl_kernel colorSpillOp(CCL_WRITE(dst),
                        CCL_READ(color),
                        CCL_READ(factor),
                        const int spill_method,
                        const int spill_channel,
                        const int channel2,
                        const int channel3,
                        const int lim_channel,
                        const float lim_scale,
                        const float3 rgb_mut,
                        const float3 uspill)
{
  READ_DECL(color);
  READ_DECL(factor);
  WRITE_DECL(dst);
  float map;
  float rfac;
  float *rgb_a;

  CPU_LOOP_START(dst);

  COPY_COORDS(color, dst_coords);
  COPY_COORDS(factor, dst_coords);

  READ_IMG4(color, color_pix);
  READ_IMG1(factor, factor_pix);

  /* store matte(alpha) value in [0] to go with
   * COM_SetAlphaOperation and the Value output
   */
  rfac = min(1.0f, factor_pix.x);

  rgb_a = (float *)&color_pix;
  if (spill_method == 0) { /* simple */
    map = rfac * (rgb_a[spill_channel] - (lim_scale * rgb_a[lim_channel]));
  }
  else { /* average */
    map = rfac * (rgb_a[spill_channel] - (lim_scale * AVG(rgb_a[channel2], rgb_a[channel3])));
  }

  if (map > 0.0f) {
    float3 rgb = make_float3_4(color_pix);
    rgb = rgb + rgb_mut * uspill * map;
    color_pix = make_float4_31(rgb, color_pix.w);
  }

  WRITE_IMG4(dst, color_pix);

  CPU_LOOP_END
}

#undef AVG

CCL_NAMESPACE_END
#undef OPENCL_CODE

void ColorSpillOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto factor = getInputOperation(1)->getPixels(this, man);
  CCL::float3 rgb_mut = CCL::make_float3(m_rmut, m_gmut, m_bmut);
  CCL::float3 uspill = CCL::make_float3(
      m_settings->uspillr, m_settings->uspillg, m_settings->uspillb);
  auto cpu_write = std::bind(CCL::colorSpillOp,
                             _1,
                             color,
                             factor,
                             m_spill_method,
                             m_spill_channel,
                             m_channel2,
                             m_channel3,
                             m_settings->limchan,
                             m_settings->limscale,
                             rgb_mut,
                             uspill);
  computeWriteSeek(man, cpu_write, "colorSpillOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*factor);
    kernel->addIntArg(m_spill_method);
    kernel->addIntArg(m_spill_channel);
    kernel->addIntArg(m_channel2);
    kernel->addIntArg(m_channel3);
    kernel->addIntArg(m_settings->limchan);
    kernel->addFloatArg(m_settings->limscale);
    kernel->addFloat3Arg(rgb_mut);
    kernel->addFloat3Arg(uspill);
  });
}
