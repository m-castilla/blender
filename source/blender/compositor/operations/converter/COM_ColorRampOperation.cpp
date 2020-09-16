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

#include "COM_ColorRampOperation.h"
#include "COM_ComputeKernel.h"

#include "COM_kernel_cpu.h"

using namespace std::placeholders;
ColorRampOperation::ColorRampOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::COLOR);

  this->m_colorBand = NULL;
}
void ColorRampOperation::hashParams()
{
  NodeOperation::hashParams();

  /*No need to hash m_colorBand->cur and m_colorBand->data->cur variables, they are used as cursor
   * not parameters*/
  hashParam(m_colorBand->color_mode);
  hashParam(m_colorBand->ipotype);
  hashParam(m_colorBand->ipotype_hue);
  hashParam(m_colorBand->tot);
  for (int i = 0; i < m_colorBand->tot; i++) {
    CBData data = m_colorBand->data[i];
    hashFloatData(&data.r, 5);
  }
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel colorRampOp(CCL_WRITE(dst),
                       CCL_READ(factor),
                       const int n_bands,
                       const int interp_type,
                       const int hue_interp_type,
                       const int color_mode,
                       ccl_constant float4 *bands_colors,
                       ccl_constant float *bands_pos)
{
  READ_DECL(factor);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(factor, dst_coords);

  READ_IMG1(factor, factor_pix);
  float4 result = colorband_evaluate(
      factor_pix.x, n_bands, interp_type, hue_interp_type, color_mode, bands_colors, bands_pos);
  WRITE_IMG4(dst, result);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ColorRampOperation::execPixels(ExecutionManager &man)
{
  auto factor = getInputOperation(0)->getPixels(this, man);

  int n_bands = m_colorBand->tot;
  CCL::float4 *colors = new CCL::float4[n_bands];
  float *positions = new float[n_bands];
  for (int i = 0; i < n_bands; i++) {
    CBData band = m_colorBand->data[i];
    colors[i] = CCL::make_float4(band.r, band.g, band.b, band.a);
    positions[i] = band.pos;
  }

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::colorRampOp,
      _1,
      factor,
      n_bands,
      m_colorBand->ipotype,
      m_colorBand->ipotype_hue,
      m_colorBand->color_mode,
      colors,
      positions);
  computeWriteSeek(man, cpu_write, "colorRampOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*factor);
    kernel->addIntArg(n_bands);
    kernel->addIntArg(m_colorBand->ipotype);
    kernel->addIntArg(m_colorBand->ipotype_hue);
    kernel->addIntArg(m_colorBand->color_mode);
    kernel->addFloat4CArrayArg(colors, n_bands);
    kernel->addFloatCArrayArg(positions, n_bands);
  });

  delete[] colors;
  delete[] positions;
}
