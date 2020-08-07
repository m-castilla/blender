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

#include "COM_ScaleOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel scaleVariableOp(CCL_WRITE(dst),
                           CCL_READ(color),
                           CCL_READ(x_input),
                           CCL_READ(y_input),
                           CCL_SAMPLER(sampler),
                           float color_width,
                           float color_height,
                           float center_x,
                           float center_y,
                           BOOL relative)
{
  SAMPLE_DECL(color);
  READ_DECL(x_input);
  READ_DECL(y_input);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(x_input, dst_coords, x_input_pix);
  READ_IMG(y_input, dst_coords, y_input_pix);

  int2 color_coords;
  if (relative) {
    color_coords.x = center_x + (dst_coords.x - center_x) / x_input_pix.x;
    color_coords.y = center_y + (dst_coords.y - center_y) / y_input_pix.x;
  }
  else {
    color_coords.x = center_x + (dst_coords.x - center_x) / (x_input_pix.x / color_width);
    color_coords.y = center_y + (dst_coords.y - center_y) / (y_input_pix.x / color_height);
  }
  SAMPLE_IMG(color, color_coords, sampler, color_pix);
  WRITE_IMG(dst, dst_coords, color_pix);

  CPU_LOOP_END
}

ccl_kernel scaleFixedOp(CCL_WRITE(dst),
                        CCL_READ(input),
                        CCL_SAMPLER(sampler),
                        BOOL has_scale_offset,
                        float scale_offset_x,
                        float scale_offset_y,
                        float scale_rel_x,
                        float scale_rel_y)
{
  SAMPLE_DECL(input);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  int2 input_coords;
  if (has_scale_offset) {
    input_coords.x = ((dst_coords.x - scale_offset_x) * scale_rel_x);
    input_coords.y = ((dst_coords.y - scale_offset_y) * scale_rel_y);
  }
  else {
    input_coords.x = dst_coords.x * scale_rel_x;
    input_coords.y = dst_coords.y * scale_rel_y;
  }

  SAMPLE_IMG(input, input_coords, sampler, input_pix);
  WRITE_IMG(dst, dst_coords, input_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

using namespace std::placeholders;

ScaleOperation::ScaleOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::DYNAMIC);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::DYNAMIC);
  this->setMainInputSocketIndex(0);
  this->m_inputOperation = NULL;
  this->m_inputXOperation = NULL;
  this->m_inputYOperation = NULL;
  m_relative = false;
  m_sampler = PixelsSampler{PixelInterpolation::BILINEAR, PixelExtend::CLIP};
}

void ScaleOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_relative);
}

void ScaleOperation::initExecution()
{
  this->m_inputOperation = this->getInputSocket(0)->getLinkedOp();
  this->m_inputXOperation = this->getInputSocket(1)->getLinkedOp();
  this->m_inputYOperation = this->getInputSocket(2)->getLinkedOp();
  NodeOperation::initExecution();
}

void ScaleOperation::deinitExecution()
{
  this->m_inputOperation = NULL;
  this->m_inputXOperation = NULL;
  this->m_inputYOperation = NULL;
  NodeOperation::deinitExecution();
}

void ScaleOperation::execPixels(ExecutionManager &man)
{
  int color_width = m_inputOperation->getWidth();
  int color_height = m_inputOperation->getHeight();
  float center_x = color_width / 2.0f;
  float center_y = color_height / 2.0f;
  auto color_input = m_inputOperation->getPixels(this, man);
  auto x_input = m_inputXOperation->getPixels(this, man);
  auto y_input = m_inputYOperation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::scaleVariableOp,
      _1,
      color_input,
      x_input,
      y_input,
      m_sampler,
      (float)color_width,
      (float)color_height,
      center_x,
      center_y,
      m_relative);
  computeWriteSeek(man, cpu_write, "scaleVariableOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color_input);
    kernel->addReadImgArgs(*x_input);
    kernel->addReadImgArgs(*y_input);
    kernel->addSamplerArg(m_sampler);
    kernel->addFloatArg(color_width);
    kernel->addFloatArg(color_height);
    kernel->addFloatArg(center_x);
    kernel->addFloatArg(center_y);
    kernel->addBoolArg(m_relative);
  });
}

// Absolute fixed size
ScaleFixedSizeOperation::ScaleFixedSizeOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::DYNAMIC, InputResizeMode::NO_RESIZE);
  this->addOutputSocket(SocketType::DYNAMIC);
  this->setMainInputSocketIndex(0);
  m_sampler = PixelsSampler{PixelInterpolation::BILINEAR, PixelExtend::CLIP};
  m_inputOperation = NULL;

  m_newWidth = 0;
  m_newHeight = 0;
  m_relX = 0;
  m_relY = 0;

  m_offsetX = 0;
  m_offsetY = 0;
  m_is_aspect = false;
  m_is_crop = false;
  m_is_offset = false;
}

void ScaleFixedSizeOperation::hashParams()
{
  hashParam(m_newWidth);
  hashParam(m_newHeight);
  hashParam(m_offsetX);
  hashParam(m_offsetY);
  hashParam(m_is_aspect);
  hashParam(m_is_crop);
  hashParam(m_is_offset);
}

void ScaleFixedSizeOperation::initExecution()
{
  this->m_inputOperation = this->getInputSocket(0)->getLinkedOp();
  this->m_relX = this->m_inputOperation->getWidth() / (float)this->m_newWidth;
  this->m_relY = this->m_inputOperation->getHeight() / (float)this->m_newHeight;

  /* *** all the options below are for a fairly special case - camera framing *** */
  if (this->m_offsetX != 0.0f || this->m_offsetY != 0.0f) {
    this->m_is_offset = true;

    if (this->m_newWidth > this->m_newHeight) {
      this->m_offsetX *= this->m_newWidth;
      this->m_offsetY *= this->m_newWidth;
    }
    else {
      this->m_offsetX *= this->m_newHeight;
      this->m_offsetY *= this->m_newHeight;
    }
  }

  if (this->m_is_aspect) {
    /* apply aspect from clip */
    const float w_src = this->m_inputOperation->getWidth();
    const float h_src = this->m_inputOperation->getHeight();

    /* destination aspect is already applied from the camera frame */
    const float w_dst = this->m_newWidth;
    const float h_dst = this->m_newHeight;

    const float asp_src = w_src / h_src;
    const float asp_dst = w_dst / h_dst;

    if (fabsf(asp_src - asp_dst) >= FLT_EPSILON) {
      if ((asp_src > asp_dst) == (this->m_is_crop == true)) {
        /* fit X */
        const float div = asp_src / asp_dst;
        this->m_relX /= div;
        this->m_offsetX += ((w_src - (w_src * div)) / (w_src / w_dst)) / 2.0f;
      }
      else {
        /* fit Y */
        const float div = asp_dst / asp_src;
        this->m_relY /= div;
        this->m_offsetY += ((h_src - (h_src * div)) / (h_src / h_dst)) / 2.0f;
      }

      this->m_is_offset = true;
    }
  }
  /* *** end framing options *** */

  NodeOperation::initExecution();
}

void ScaleFixedSizeOperation::deinitExecution()
{
  this->m_inputOperation = NULL;
  NodeOperation::deinitExecution();
}

void ScaleFixedSizeOperation::execPixels(ExecutionManager &man)
{
  auto input = m_inputOperation->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::scaleFixedOp,
      _1,
      input,
      m_sampler,
      m_is_offset,
      m_offsetX,
      m_offsetY,
      m_relX,
      m_relY);
  computeWriteSeek(man, cpu_write, "scaleFixedOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input);
    kernel->addSamplerArg(m_sampler);
    kernel->addBoolArg(m_is_offset);
    kernel->addFloatArg(m_offsetX);
    kernel->addFloatArg(m_offsetY);
    kernel->addFloatArg(m_relX);
    kernel->addFloatArg(m_relY);
  });
}

void ScaleFixedSizeOperation::determineResolution(int resolution[2],
                                                  int /*preferredResolution*/[2],
                                                  bool setResolution)
{
  int nr[2];
  nr[0] = this->m_newWidth;
  nr[1] = this->m_newHeight;
  if (setResolution) {
    NodeOperation::determineResolution(resolution, nr, setResolution);
  }
  resolution[0] = this->m_newWidth;
  resolution[1] = this->m_newHeight;
}
