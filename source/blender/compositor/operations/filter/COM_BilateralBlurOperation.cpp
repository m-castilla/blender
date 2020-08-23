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

#include "COM_BilateralBlurOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_PixelsUtil.h"
#include "COM_Rect.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
BilateralBlurOperation::BilateralBlurOperation() : NodeOperation(), QualityStepHelper()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
  m_data = nullptr;
  m_space = 0.0f;
  m_space_ceil = 0;
}
void BilateralBlurOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_data->sigma_space);
  hashParam(m_data->sigma_color);
  hashParam(m_data->iter);
  hashParam((int)QualityStepHelper::getQuality());
}

void BilateralBlurOperation::initExecution()
{
  m_space = this->m_data->sigma_space + this->m_data->iter;
  m_space_ceil = ceilf(m_space);
  QualityStepHelper::initExecution(QualityHelper::INCREASE);
  NodeOperation::initExecution();
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel bilateralBlurDetermOp(CCL_WRITE(dst),
                                 CCL_READ(color),
                                 CCL_READ(determ),
                                 int max_x,
                                 int max_y,
                                 int space_ceil,
                                 int quality_step,
                                 float sigma_color)
{
  READ_DECL(color);
  READ_DECL(determ);
  WRITE_DECL(dst);
  float4 determ_ref_pix;
  // int2 determ_coords;
  float4 blur_pix;

  CPU_LOOP_START(dst);

  COPY_COORDS(determ, dst_coords);
  READ_IMG4(determ, determ_ref_pix);

  int minx = max(dst_coords.x - space_ceil, 0);      // same as floor(x - m_space);
  int maxx = min(dst_coords.x + space_ceil, max_x);  // same as ceil(x + m_space);
  int miny = max(dst_coords.y - space_ceil, 0);      // same as floor(y - m_space);
  int maxy = min(dst_coords.y + space_ceil, max_y);  // same as ceil(y + m_space);

  float deltaColor;
  float blurDivider = 0.0f;
  blur_pix = make_float4_1(0.0f);

  /* TODO(sergey): This isn't really good bilateral filter, it should be
   * using gaussian bell for weights. Also sigma_color doesn't seem to be
   * used correct at all.
   */
  SET_COORDS(determ, minx, miny);
  SET_COORDS(color, minx, miny);
  for (int yi = miny; yi < maxy; yi += quality_step) {
    for (int xi = minx; xi < maxx; xi += quality_step) {
      // read determinator
      READ_IMG4(determ, determ_pix);
      deltaColor = fabsf(determ_ref_pix.x - determ_pix.x) +
                   fabsf(determ_ref_pix.y - determ_pix.y) +
                   fabsf(determ_ref_pix.z -
                         determ_pix.z);  // do not take the alpha channel into account
      if (deltaColor < sigma_color) {
        // add this to the blur
        UPDATE_COORDS_X(color, xi);
        READ_IMG4(color, color_pix);
        blur_pix += color_pix;
        blurDivider += 1.0f;
      }
      INCR_COORDS_X(determ, quality_step);
    }
    INCR_COORDS_Y(determ, quality_step);
    INCR_COORDS_Y(color, quality_step);
    UPDATE_COORDS_X(determ, minx);
  }

  if (blurDivider > 0.0f) {
    blur_pix *= 1.0f / blurDivider;
  }
  else {
    blur_pix.w = 1.0f;
  }

  WRITE_IMG4(dst, blur_pix);

  CPU_LOOP_END
}

ccl_kernel bilateralBlurOp(
    CCL_WRITE(dst), CCL_READ(color), int max_x, int max_y, int space_ceil, int quality_step)
{
  READ_DECL(color);
  WRITE_DECL(dst);
  float4 blur_pix;

  CPU_LOOP_START(dst);

  COPY_COORDS(color, dst_coords);

  int minx = max(dst_coords.x - space_ceil, 0);      // same as floor(x - m_space);
  int maxx = min(dst_coords.x + space_ceil, max_x);  // same as ceil(x + m_space);
  int miny = max(dst_coords.y - space_ceil, 0);      // same as floor(y - m_space);
  int maxy = min(dst_coords.y + space_ceil, max_y);  // same as ceil(y + m_space);

  blur_pix = make_float4_1(0.0f);
  float blurDivider = (maxx - minx) * (maxy - miny);
  /* TODO(sergey): This isn't really good bilateral filter, it should be
   * using gaussian bell for weights. Also sigma_color doesn't seem to be
   * used correct at all.
   */
  for (int yi = miny; yi < maxy; yi += quality_step) {
    UPDATE_COORDS_Y(color, yi);
    for (int xi = minx; xi < maxx; xi += quality_step) {
      // read determinator
      UPDATE_COORDS_X(color, xi);
      READ_IMG(color, color_pix);
      blur_pix += color_pix;
    }
  }

  blur_pix *= 1.0f / blurDivider;
  WRITE_IMG(dst, blur_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void BilateralBlurOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto determ = getInputOperation(1)->getPixels(this, man);

  if (!man.canExecPixels() || determ->is_single_elem) {
    std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
        CCL::bilateralBlurOp,
        _1,
        color,
        getWidth(),
        getHeight(),
        m_space_ceil,
        QualityStepHelper::getStep());
    computeWriteSeek(man, cpu_write, "bilateralBlurOp", [&](ComputeKernel *kernel) {
      kernel->addReadImgArgs(*color);
      kernel->addIntArg(getWidth());
      kernel->addIntArg(getHeight());
      kernel->addIntArg(m_space_ceil);
      kernel->addIntArg(QualityStepHelper::getStep());
    });
  }
  else {
    std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
        CCL::bilateralBlurDetermOp,
        _1,
        color,
        determ,
        getWidth(),
        getHeight(),
        m_space_ceil,
        QualityStepHelper::getStep(),
        m_data->sigma_color);
    computeWriteSeek(man, cpu_write, "bilateralBlurDetermOp", [&](ComputeKernel *kernel) {
      kernel->addReadImgArgs(*color);
      kernel->addReadImgArgs(*determ);
      kernel->addIntArg(getWidth());
      kernel->addIntArg(getHeight());
      kernel->addIntArg(m_space_ceil);
      kernel->addIntArg(QualityStepHelper::getStep());
      kernel->addIntArg(m_data->sigma_color);
    });
  }
}
