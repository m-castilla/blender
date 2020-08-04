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

#include "COM_HueSaturationValueCorrectOperation.h"
#include "BKE_colortools.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu_nocompat.h"

HueSaturationValueCorrectOperation::HueSaturationValueCorrectOperation() : CurveBaseOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
}
void HueSaturationValueCorrectOperation::initExecution()
{
  CurveBaseOperation::initExecution();
}

void HueSaturationValueCorrectOperation::deinitExecution()
{
  CurveBaseOperation::deinitExecution();
}

void HueSaturationValueCorrectOperation::execPixels(ExecutionManager &man)
{
  auto hsv = getInputOperation(0)->getPixels(this, man);

  auto cpu_write = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    CPU_READ_DECL(hsv);
    CPU_WRITE_DECL(dst);
    CPU_LOOP_START(dst);

    CPU_READ_OFFSET(hsv, dst);
    CPU_WRITE_OFFSET(dst);

    float f;
    float *hsv_val = &hsv_img.buffer[hsv_offset];
    float result[4];

    /* adjust hue, scaling returned default 0.5 up to 1 */
    f = BKE_curvemapping_evaluateF(this->m_curveMapping, 0, hsv_val[0]);
    result[0] = hsv_val[0] + (f - 0.5f);

    /* adjust saturation, scaling returned default 0.5 up to 1 */
    f = BKE_curvemapping_evaluateF(this->m_curveMapping, 1, hsv_val[0]);
    result[1] = hsv_val[1] * (f * 2.0f);

    /* adjust value, scaling returned default 0.5 up to 1 */
    f = BKE_curvemapping_evaluateF(this->m_curveMapping, 2, hsv_val[0]);
    result[2] = hsv_val[2] * (f * 2.0f);

    result[0] = result[0] - floorf(result[0]); /* mod 1.0 */
    CLAMP(result[1], 0.0f, 1.0f);

    dst_img.buffer[dst_offset] = result[0];
    dst_img.buffer[dst_offset + 1] = result[1];
    dst_img.buffer[dst_offset + 2] = result[2];
    dst_img.buffer[dst_offset + 3] = hsv_img.buffer[hsv_offset + 3];

    CPU_LOOP_END;
  };
  cpuWriteSeek(man, cpu_write);
}
