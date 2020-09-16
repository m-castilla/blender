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

#include "COM_TransformOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_GlobalManager.h"
#include "COM_kernel_cpu.h"

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel transformOp(CCL_WRITE(dst),
                       CCL_READ(color),
                       CCL_SAMPLER(sampler),
                       float scale_center_x,
                       float scale_center_y,
                       float scale_x,
                       float scale_y,
                       float rotate_center_x,
                       float rotate_center_y,
                       float rad_cosine,
                       float rad_sine,
                       float translate_x,
                       float translate_y)
{
  READ_DECL(color);
  WRITE_DECL(dst);

  float2 read_coordsf;

  CPU_LOOP_START(dst);

  // scale
  read_coordsf.x = scale_center_x + (dst_coords.x - scale_center_x) / scale_x;
  read_coordsf.y = scale_center_y + (dst_coords.y - scale_center_y) / scale_y;

  // rotate
  const float dx = read_coordsf.x - rotate_center_x;
  const float dy = read_coordsf.y - rotate_center_y;
  read_coordsf.x = rotate_center_x + (rad_cosine * dx + rad_sine * dy);
  read_coordsf.y = rotate_center_y + (-rad_sine * dx + rad_cosine * dy);

  // translate
  read_coordsf.x = read_coordsf.x - translate_x;
  read_coordsf.y = read_coordsf.y - translate_y;

  COPY_SAMPLE_COORDS(color, read_coordsf);
  SAMPLE_IMG(color, sampler, color_pix);
  WRITE_IMG(dst, color_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

using namespace std::placeholders;

TransformOperation::TransformOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::DYNAMIC);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::DYNAMIC);
  this->setMainInputSocketIndex(0);
  m_sampler = GlobalMan->getContext()->getDefaultSampler();
}

void TransformOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_sampler);
}

void TransformOperation::execPixels(ExecutionManager &man)
{
  auto color_op = getInputOperation(0);
  auto color_input = color_op->getPixels(this, man);
  auto x_translate_pix = getInputOperation(1)->getSinglePixel(this, man, 0, 0);
  auto y_translate_pix = getInputOperation(2)->getSinglePixel(this, man, 0, 0);
  auto radians_pix = getInputOperation(3)->getSinglePixel(this, man, 0, 0);
  auto scale_pix = getInputOperation(4)->getSinglePixel(this, man, 0, 0);

  int color_width = color_op->getWidth();
  int color_height = color_op->getHeight();
  float scale_center_x = color_width / 2.0f;
  float scale_center_y = color_height / 2.0f;
  float rotate_center_x = (color_width - 1) / 2.0f;
  float rotate_center_y = (color_height - 1) / 2.0f;

  float scale_x = 1.0f, scale_y = 1.0f;
  if (scale_pix) {
    scale_x = scale_pix[0];
    scale_y = scale_pix[0];
  }

  float radians = 0.0f;
  if (radians_pix) {
    radians = radians_pix[0];
  }
  const float cosine = cos(radians);
  const float sine = sin(radians);

  float translate_x = 0.0f, translate_y = 0.0f;
  if (x_translate_pix) {
    translate_x = x_translate_pix[0];
  }
  if (y_translate_pix) {
    translate_y = y_translate_pix[0];
  }

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::transformOp,
      _1,
      color_input,
      m_sampler,
      scale_center_x,
      scale_center_y,
      scale_x,
      scale_y,
      rotate_center_x,
      rotate_center_y,
      cosine,
      sine,
      translate_x,
      translate_y);
  computeWriteSeek(man, cpu_write, "transformOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color_input);
    kernel->addSamplerArg(m_sampler);
    kernel->addFloatArg(scale_center_x);
    kernel->addFloatArg(scale_center_y);
    kernel->addFloatArg(scale_x);
    kernel->addFloatArg(scale_y);
    kernel->addFloatArg(rotate_center_x);
    kernel->addFloatArg(rotate_center_y);
    kernel->addFloatArg(cosine);
    kernel->addFloatArg(sine);
    kernel->addFloatArg(translate_x);
    kernel->addFloatArg(translate_y);
  });
}
