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

#include "COM_CropOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_kernel_cpu.h"
#include <algorithm>

using namespace std::placeholders;
CropBaseOperation::CropBaseOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::DYNAMIC, InputResizeMode::NO_RESIZE);
  this->addOutputSocket(SocketType::DYNAMIC);
  this->m_settings = NULL;
  m_relative = false;
}

void CropBaseOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_relative);
  if (m_relative) {
    hashParam(m_settings->fac_x1);
    hashParam(m_settings->fac_x2);
    hashParam(m_settings->fac_y1);
    hashParam(m_settings->fac_y2);
  }
  else {
    hashParam(m_settings->x1);
    hashParam(m_settings->x2);
    hashParam(m_settings->y1);
    hashParam(m_settings->y2);
  }
}

rcti CropBaseOperation::getCropRectForInput(int input_w, int input_h)
{
  NodeTwoXYs local_settings = *this->m_settings;

  rcti rect;
  if (input_w > 0.0f && input_h > 0.0f) {
    int x1, x2, y1, y2;
    if (this->m_relative) {
      x1 = input_w * local_settings.fac_x1;
      x2 = input_w * local_settings.fac_x2;
      y1 = input_h * local_settings.fac_y1;
      y2 = input_h * local_settings.fac_y2;
    }
    else {
      x1 = local_settings.x1;
      x2 = local_settings.x2;
      y1 = local_settings.y1;
      y2 = local_settings.y2;
    }
    if (input_w <= x1 + 1) {
      x1 = input_w - 1;
    }
    if (input_h <= y1 + 1) {
      y1 = input_h - 1;
    }
    if (input_w <= x2 + 1) {
      x2 = input_w - 1;
    }
    if (input_h <= y2 + 1) {
      y2 = input_h - 1;
    }

    rect.xmax = std::max(x1, x2) + 1;
    rect.xmin = std::min(x1, x2);
    rect.ymax = std::max(y1, y2) + 1;
    rect.ymin = std::min(y1, y2);
  }
  else {
    rect.xmax = 0;
    rect.xmin = 0;
    rect.ymax = 0;
    rect.ymin = 0;
  }
  return rect;
}

CropOperation::CropOperation() : CropBaseOperation()
{
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel cropOp(CCL_WRITE(dst),
                  CCL_READ(color),
                  const int xmin,
                  const int xmax,
                  const int ymin,
                  const int ymax)
{
  READ_DECL(color);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  if ((dst_coords.x < xmax && dst_coords.x >= xmin) &&
      (dst_coords.y < ymax && dst_coords.y >= ymin)) {
    COPY_COORDS(color, dst_coords);
    READ_IMG(color, color_pix);
  }
  else {
    color_pix = make_float4_1(0.0f);
  }

  WRITE_IMG(dst, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void CropOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  rcti rect;
  if (man.canExecPixels()) {
    rect = getCropRectForInput(color->getWidth(), color->getHeight());
  }
  auto cpu_write = std::bind(CCL::cropOp, _1, color, rect.xmin, rect.xmax, rect.ymin, rect.ymax);
  computeWriteSeek(man, cpu_write, "cropOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addIntArg(rect.xmin);
    kernel->addIntArg(rect.xmax);
    kernel->addIntArg(rect.ymin);
    kernel->addIntArg(rect.ymax);
  });
}

CropImageOperation::CropImageOperation() : CropBaseOperation()
{
  /* pass */
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel cropImageOp(CCL_WRITE(dst),
                       CCL_READ(color),
                       const int xmin,
                       const int ymin,
                       const int width,
                       const int height)
{
  READ_DECL(color);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  if (dst_coords.x >= 0 && dst_coords.x < width && dst_coords.y >= 0 && dst_coords.y < height) {
    SET_COORDS(color, (dst_coords.x + xmin), (dst_coords.y + ymin));
    READ_IMG(color, color_pix);
  }
  else {
    color_pix = make_float4_1(0.0f);
  }

  WRITE_IMG(dst, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void CropImageOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  rcti rect;
  if (man.canExecPixels()) {
    rect = getCropRectForInput(color->getWidth(), color->getHeight());
  }
  int w = getWidth();
  int h = getHeight();
  auto cpu_write = std::bind(CCL::cropImageOp, _1, color, rect.xmin, rect.ymin, w, h);
  computeWriteSeek(man, cpu_write, "cropImageOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addIntArg(rect.xmin);
    kernel->addIntArg(rect.ymin);
    kernel->addIntArg(w);
    kernel->addIntArg(h);
  });
}

ResolutionType CropImageOperation::determineResolution(int resolution[2],
                                                       int preferredResolution[2],

                                                       bool setResolution)
{
  NodeOperation::determineResolution(resolution, preferredResolution, setResolution);

  rcti crop_rect = getCropRectForInput(resolution[0], resolution[1]);

  resolution[0] = crop_rect.xmax - crop_rect.xmin;
  resolution[1] = crop_rect.ymax - crop_rect.ymin;

  return ResolutionType::Determined;
}
