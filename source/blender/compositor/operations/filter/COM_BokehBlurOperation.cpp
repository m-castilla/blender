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

#include "COM_BokehBlurOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_kernel_cpu.h"
#include <functional>

using namespace std::placeholders;
BokehBlurOperation::BokehBlurOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::COLOR, InputResizeMode::NO_RESIZE);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::COLOR);

  this->m_size = 1.0f;
  this->m_extend_bounds = false;
}
void BokehBlurOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_size);
  hashParam(m_extend_bounds);
  hashParam((int)QualityStepHelper::getQuality());
}

void BokehBlurOperation::initExecution()
{
  QualityStepHelper::initExecution(QualityHelper::INCREASE);
  NodeOperation::initExecution();
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel bokehBlurOp(CCL_WRITE(dst),
                       CCL_READ(color),
                       CCL_READ(bokeh),
                       CCL_READ(size),
                       CCL_READ(bounding),
                       CCL_SAMPLER(sampler),
                       const int pixel_size,
                       const float bokeh_factor,
                       const int bokeh_mid_x,
                       const int bokeh_mid_y,
                       const int quality_step)
{
  READ_DECL(color);
  READ_DECL(bokeh);
  READ_DECL(size);
  READ_DECL(bounding);
  WRITE_DECL(dst);
  float4 color_accum, multiplier_accum;
  int2 blur_coords;

  CPU_LOOP_START(dst);

  COPY_COORDS(bounding, dst_coords);
  READ_IMG1(bounding, bounding_pix);
  if (bounding_pix.x > 0.0f) {
    multiplier_accum = make_float4_1(0.0f);
    color_accum = make_float4_1(0.0f);

    if (pixel_size < 2) {
      COPY_COORDS(color, dst_coords);
      READ_IMG4(color, color_accum);
      multiplier_accum = make_float4_1(1.0f);
    }
    const int blur_miny = dst_coords.y - pixel_size;
    const int blur_maxy = dst_coords.y + pixel_size;
    const int blur_minx = dst_coords.x - pixel_size;
    const int blur_maxx = dst_coords.x + pixel_size;
    SET_SAMPLE_COORDS(bokeh, blur_minx, blur_miny);
    SET_SAMPLE_COORDS(color, blur_minx, blur_miny);
    float bokeh_coordf_y, bokeh_coordf_x;
    for (blur_coords.y = blur_miny; blur_coords.y < blur_maxy; blur_coords.y += quality_step) {
      bokeh_coordf_y = bokeh_mid_y - (blur_coords.y - dst_coords.y) * bokeh_factor;
      UPDATE_SAMPLE_COORDS_Y(bokeh, bokeh_coordf_y);
      for (blur_coords.x = blur_minx; blur_coords.x < blur_maxx; blur_coords.x += quality_step) {
        bokeh_coordf_x = bokeh_mid_x - (blur_coords.x - dst_coords.x) * bokeh_factor;
        UPDATE_SAMPLE_COORDS_X(bokeh, bokeh_coordf_x);
        SAMPLE_IMG(bokeh, sampler, bokeh_pix);
        SAMPLE_IMG(color, sampler, color_pix);
        color_accum += bokeh_pix * color_pix;
        multiplier_accum += bokeh_pix;
        INCR_SAMPLE_COORDS_X(bokeh, quality_step);
        INCR_SAMPLE_COORDS_X(color, quality_step);
      }
      INCR_SAMPLE_COORDS_Y(bokeh, quality_step);
      INCR_SAMPLE_COORDS_Y(color, quality_step);
      UPDATE_SAMPLE_COORDS_X(bokeh, blur_minx);
      UPDATE_SAMPLE_COORDS_X(color, blur_minx);
    }

    color_pix = color_accum * (1.0f / multiplier_accum);
    WRITE_IMG4(dst, color_pix);
  }
  else {
    COPY_COORDS(color, dst_coords);
    READ_IMG4(color, color_pix);
    WRITE_IMG4(dst, color_pix);
  }

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void BokehBlurOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto bokeh = getInputOperation(1)->getPixels(this, man);
  auto size = getInputOperation(2)->getPixels(this, man);
  auto bounding = getInputOperation(3)->getPixels(this, man);
  float bokehMidX, bokehMidY;
  int pixel_size;
  float bokeh_factor;
  int quality_step = QualityStepHelper::getStep();
  PixelsSampler sampler = {PixelInterpolation::NEAREST, PixelExtend::CLIP};

  if (man.canExecPixels()) {
    int bokeh_width = bokeh->getWidth();
    int bokeh_height = bokeh->getHeight();
    int output_width = getWidth();
    int output_height = getHeight();
    bokehMidX = bokeh_width / 2.0f;
    bokehMidY = bokeh_height / 2.0f;

    const float max_out_dim = CCL::max(output_width, output_height);
    pixel_size = m_size * max_out_dim / 100.0f;

    float bokeh_dim = CCL::min(bokeh_width, bokeh_height) / 2.0f;
    bokeh_factor = bokeh_dim / pixel_size;
  }

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::bokehBlurOp,
      _1,
      color,
      bokeh,
      size,
      bounding,
      sampler,
      pixel_size,
      bokeh_factor,
      bokehMidX,
      bokehMidY,
      quality_step);
  computeWriteSeek(man, cpu_write, "bokehBlurOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*bokeh);
    kernel->addReadImgArgs(*size);
    kernel->addReadImgArgs(*bounding);
    kernel->addSamplerArg(sampler);
    kernel->addIntArg(pixel_size);
    kernel->addFloatArg(bokeh_factor);
    kernel->addIntArg(bokehMidX);
    kernel->addIntArg(bokehMidY);
    kernel->addIntArg(quality_step);
  });
}

ResolutionType BokehBlurOperation::determineResolution(int resolution[2],
                                                       int preferredResolution[2],
                                                       bool setResolution)
{
  NodeOperation::determineResolution(resolution, preferredResolution, setResolution);
  if (this->m_extend_bounds) {
    const float max_dim = CCL::max(resolution[0], resolution[1]);
    resolution[0] += 2 * this->m_size * max_dim / 100.0f;
    resolution[1] += 2 * this->m_size * max_dim / 100.0f;
  }
  return ResolutionType::Determined;
}
