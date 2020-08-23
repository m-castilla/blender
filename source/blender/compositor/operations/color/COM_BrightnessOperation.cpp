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

#include "COM_BrightnessOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_PixelsUtil.h"
#include "COM_Rect.h"
#include "COM_kernel_cpu.h"

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel brightnessOp(
    CCL_WRITE(dst), CCL_READ(color), CCL_READ(bright), CCL_READ(contrast), BOOL premultiply)
{
  float a, b, bright_value, contrast_value, delta;

  READ_DECL(color);
  READ_DECL(bright);
  READ_DECL(contrast);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(color, dst_coords);
  COPY_COORDS(bright, dst_coords);
  COPY_COORDS(contrast, dst_coords);

  READ_IMG4(color, color_pix);
  READ_IMG1(bright, bright_pix);
  READ_IMG1(contrast, contrast_pix);

  bright_value = bright_pix.x / 100.0f;
  contrast_value = contrast_pix.x;
  delta = contrast_value / 200.0f;
  /*
   * The algorithm is by Werner D. Streidt
   * (http://visca.com/ffactory/archives/5-99/msg00021.html)
   * Extracted of OpenCV demhist.c
   */
  if (contrast_value > 0) {
    a = 1.0f - delta * 2.0f;
    a = 1.0f / fmaxf(a, FLT_EPSILON);
    b = a * (bright_value - delta);
  }
  else {
    delta *= -1;
    a = fmaxf(1.0f - delta * 2.0f, 0.0f);
    b = a * bright_value + delta;
  }

  float alpha = color_pix.w;
  if (premultiply) {
    premul_to_straight(color_pix);
    color_pix = a * color_pix + b;
    color_pix.w = alpha;
    straight_to_premul(color_pix);
  }
  else {
    color_pix = a * color_pix + b;
    color_pix.w = alpha;
  }

  WRITE_IMG4(dst, color_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

using namespace std::placeholders;

BrightnessOperation::BrightnessOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::COLOR);
  this->m_use_premultiply = false;
}
void BrightnessOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_use_premultiply);
}
void BrightnessOperation::setUsePremultiply(bool use_premultiply)
{
  this->m_use_premultiply = use_premultiply;
}

void BrightnessOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto bright = getInputOperation(1)->getPixels(this, man);
  auto contrast = getInputOperation(2)->getPixels(this, man);
  bool premultiply = this->m_use_premultiply;
  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::brightnessOp, _1, color, bright, contrast, premultiply);
  computeWriteSeek(man, cpu_write, "brightnessOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*bright);
    kernel->addReadImgArgs(*contrast);
    kernel->addBoolArg(m_use_premultiply);
  });
}
