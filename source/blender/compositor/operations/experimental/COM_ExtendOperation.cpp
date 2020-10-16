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

#include "COM_ExtendOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_GlobalManager.h"
#include "COM_kernel_cpu.h"
#include <algorithm>

using namespace std::placeholders;
ExtendOperation::ExtendOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::DYNAMIC, InputResizeMode::NO_RESIZE);
  this->addOutputSocket(SocketType::DYNAMIC);
}

void ExtendOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_add_extend_w);
  hashParam(m_add_extend_h);
  hashParam(m_scale);
  hashParam(m_extend_mode);
  hashParam(m_read_offset_x);
  hashParam(m_read_offset_y);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel extendOp(CCL_WRITE(dst),
                    CCL_READ(color),
                    CCL_SAMPLER(sampler),
                    int read_offset_x,
                    int read_offset_y,
                    float center_x,
                    float center_y,
                    float scale,
                    BOOL needs_norm_sample)
{
  READ_DECL(color);
  WRITE_DECL(dst);

  float2 read_coordsf;
  CPU_LOOP_START(dst);

  read_coordsf.x = dst_coords.x + read_offset_x;
  read_coordsf.y = dst_coords.y + read_offset_y;
  read_coordsf.x = center_x + (read_coordsf.x - center_x) / scale;
  read_coordsf.y = center_y + (read_coordsf.y - center_y) / scale;
  COPY_SAMPLE_COORDS(color, read_coordsf);
  if (needs_norm_sample) {
    SAMPLE_NORM_IMG(color, sampler, color_pix);
  }
  else {
    SAMPLE_IMG(color, sampler, color_pix);
  }

  WRITE_IMG(dst, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ExtendOperation::execPixels(ExecutionManager &man)
{
  auto color_op = getInputOperation(0);
  auto color = color_op->getPixels(this, man);
  auto interp = m_scale == 1.0f ? PixelInterpolation::NEAREST : PixelInterpolation::BILINEAR;
  PixelsSampler sampler = PixelsSampler{interp, m_extend_mode};
  int color_width = color_op->getWidth();
  int color_height = color_op->getHeight();
  float center_x = color_width / 2.0f;
  float center_y = color_height / 2.0f;
  bool needs_norm_sample = m_extend_mode == PixelExtend::REPEAT ||
                           m_extend_mode == PixelExtend::MIRROR;
  auto cpu_write = std::bind(CCL::extendOp,
                             _1,
                             color,
                             sampler,
                             m_read_offset_x,
                             m_read_offset_y,
                             center_x,
                             center_y,
                             m_scale,
                             needs_norm_sample);
  computeWriteSeek(man, cpu_write, "extendOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addSamplerArg(sampler);
    kernel->addIntArg(m_read_offset_x);
    kernel->addIntArg(m_read_offset_y);
    kernel->addFloatArg(center_x);
    kernel->addFloatArg(center_y);
    kernel->addFloatArg(m_scale);
    kernel->addBoolArg(needs_norm_sample);
  });
}

ResolutionType ExtendOperation::determineResolution(int resolution[2],
                                                    int preferredResolution[2],
                                                    bool setResolution)
{
  NodeOperation::determineResolution(resolution, preferredResolution, setResolution);

  int orig_w = resolution[0];
  int orig_h = resolution[1];
  resolution[0] *= m_add_extend_w;
  resolution[1] *= m_add_extend_h;
  if (resolution[0] > GlobalMan->getContext()->getMaxImgW()) {
    resolution[0] = GlobalMan->getContext()->getMaxImgW();
  }
  if (resolution[1] > GlobalMan->getContext()->getMaxImgH()) {
    resolution[1] = GlobalMan->getContext()->getMaxImgH();
  }
  m_read_offset_x = (orig_w - resolution[0]) / 2.0f;
  m_read_offset_y = (orig_h - resolution[1]) / 2.0f;

  return ResolutionType::Determined;
}
