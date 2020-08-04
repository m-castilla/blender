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

#include "COM_ColorCurveOperation.h"
#include "BKE_colortools.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu_nocompat.h"

ColorCurveOperation::ColorCurveOperation() : CurveBaseOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);

  this->setMainInputSocketIndex(1);
}
void ColorCurveOperation::initExecution()
{
  CurveBaseOperation::initExecution();
  BKE_curvemapping_premultiply(this->m_curveMapping, 0);
}

void ColorCurveOperation::execPixels(ExecutionManager &man)
{
  auto factor = getInputOperation(0)->getPixels(this, man);
  auto color = getInputOperation(1)->getPixels(this, man);
  auto black = getInputOperation(2)->getPixels(this, man);
  auto white = getInputOperation(3)->getPixels(this, man);

  auto cpu_write = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    CPU_READ_DECL(color);
    CPU_READ_DECL(factor);
    CPU_READ_DECL(black);
    CPU_READ_DECL(white);
    CPU_WRITE_DECL(dst);
    CPU_LOOP_START(dst);

    CPU_READ_OFFSET(color, dst);
    CPU_READ_OFFSET(factor, dst);
    CPU_READ_OFFSET(black, dst);
    CPU_READ_OFFSET(white, dst);
    CPU_WRITE_OFFSET(dst);

    CurveMapping *cumap = this->m_curveMapping;

    float bwmul[3];
    float output[4];

    float fac = factor_img.buffer[factor_offset];

    /* get our own local bwmul value,
     * since we can't be threadsafe and use cumap->bwmul & friends */
    BKE_curvemapping_set_black_white_ex(
        &black_img.buffer[black_offset], &white_img.buffer[white_offset], bwmul);

    if (fac >= 1.0f) {
      BKE_curvemapping_evaluate_premulRGBF_ex(
          cumap, output, &color_img.buffer[color_offset], &black_img.buffer[black_offset], bwmul);
    }
    else if (fac <= 0.0f) {
      copy_v3_v3(output, &color_img.buffer[color_offset]);
    }
    else {
      float col[4];
      BKE_curvemapping_evaluate_premulRGBF_ex(
          cumap, col, &color_img.buffer[color_offset], &black_img.buffer[black_offset], bwmul);
      interp_v3_v3v3(output, &color_img.buffer[color_offset], col, fac);
    }
    output[3] = color_img.buffer[color_offset + 3];

    copy_v4_v4(&dst_img.buffer[dst_offset], output);

    CPU_LOOP_END;
  };
  cpuWriteSeek(man, cpu_write);
}

void ColorCurveOperation::deinitExecution()
{
  CurveBaseOperation::deinitExecution();
}

// Constant level curve mapping
void ConstantLevelColorCurveOperation::hashParams()
{
  CurveBaseOperation::hashParams();
  hashDataAsParam(m_black, 3);
  hashDataAsParam(m_white, 3);
}

ConstantLevelColorCurveOperation::ConstantLevelColorCurveOperation() : CurveBaseOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);

  this->setMainInputSocketIndex(1);
}
void ConstantLevelColorCurveOperation::initExecution()
{
  CurveBaseOperation::initExecution();
  BKE_curvemapping_premultiply(this->m_curveMapping, 0);
  BKE_curvemapping_set_black_white(this->m_curveMapping, this->m_black, this->m_white);
}

void ConstantLevelColorCurveOperation::deinitExecution()
{
  CurveBaseOperation::deinitExecution();
}

void ConstantLevelColorCurveOperation::execPixels(ExecutionManager &man)
{
  auto factor = getInputOperation(0)->getPixels(this, man);
  auto color = getInputOperation(1)->getPixels(this, man);

  auto cpu_write = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    CPU_READ_DECL(color);
    CPU_READ_DECL(factor);
    CPU_WRITE_DECL(dst);
    CPU_LOOP_START(dst);

    CPU_READ_OFFSET(color, dst);
    CPU_READ_OFFSET(factor, dst);
    CPU_WRITE_OFFSET(dst);

    float output[4];
    float fac = factor_img.buffer[factor_offset];

    if (fac >= 1.0f) {
      BKE_curvemapping_evaluate_premulRGBF(
          this->m_curveMapping, output, &color_img.buffer[color_offset]);
    }
    else if (fac <= 0.0f) {
      copy_v3_v3(output, &color_img.buffer[color_offset]);
    }
    else {
      float col[4];
      BKE_curvemapping_evaluate_premulRGBF(
          this->m_curveMapping, col, &color_img.buffer[color_offset]);
      interp_v3_v3v3(output, &color_img.buffer[color_offset], col, fac);
    }
    output[3] = color_img.buffer[color_offset + 3];

    CPU_LOOP_END;
  };
  cpuWriteSeek(man, cpu_write);
}
