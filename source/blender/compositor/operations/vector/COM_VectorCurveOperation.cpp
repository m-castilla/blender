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

#include "COM_VectorCurveOperation.h"
#include "BKE_colortools.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

VectorCurveOperation::VectorCurveOperation() : CurveBaseOperation()
{
  this->addInputSocket(SocketType::VECTOR);
  this->addOutputSocket(SocketType::VECTOR);
}

void VectorCurveOperation::execPixels(ExecutionManager &man)
{
  auto vector = getInputOperation(0)->getPixels(this, man);

  auto cpu_write = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    READ_DECL(vector);
    WRITE_DECL(dst);
    CCL::float3 dst_pix;

    CPU_LOOP_START(dst);

    COPY_COORDS(vector, dst_coords);

    BKE_curvemapping_evaluate_premulRGBF(m_curveMapping, (float *)&dst_pix, (float *)&vector);

    WRITE_IMG3(dst, dst_pix);

    CPU_LOOP_END;
  };
  cpuWriteSeek(man, cpu_write);
}