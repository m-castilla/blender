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

#include "COM_DifferenceMatteOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
DifferenceMatteOperation::DifferenceMatteOperation() : NodeOperation()
{
  addInputSocket(SocketType::COLOR);
  addInputSocket(SocketType::COLOR);
  addOutputSocket(SocketType::VALUE);
}

void DifferenceMatteOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_settings->t1);
  hashParam(m_settings->t2);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel differenceMatteOp(
    CCL_WRITE(dst), CCL_READ(color1), CCL_READ(color2), const float tolerance, const float falloff)
{
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);
  float4 out;

  float difference;
  float alpha;

  CPU_LOOP_START(dst);

  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  difference = (fabsf(color2_pix.x - color1_pix.x) + fabsf(color2_pix.y - color1_pix.y) +
                fabsf(color2_pix.z - color1_pix.z));

  /* average together the distances */
  difference = difference / 3.0f;

  /* make 100% transparent */
  if (difference <= tolerance) {
    out.x = 0.0f;
  }
  /*in the falloff region, make partially transparent */
  else if (difference <= falloff + tolerance) {
    difference = difference - tolerance;
    alpha = difference / falloff;
    /*only change if more transparent than before */
    if (alpha < color1_pix.w) {
      out.x = alpha;
    }
    else { /* leave as before */
      out.x = color1_pix.w;
    }
  }
  else {
    /* foreground object */
    out.x = color1_pix.w;
  }

  WRITE_IMG4(dst, out);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void DifferenceMatteOperation::execPixels(ExecutionManager &man)
{
  auto color1 = getInputOperation(0)->getPixels(this, man);
  auto color2 = getInputOperation(1)->getPixels(this, man);
  const float tolerance = m_settings->t1;
  const float falloff = m_settings->t2;
  auto cpu_write = std::bind(CCL::differenceMatteOp, _1, color1, color2, tolerance, falloff);
  computeWriteSeek(man, cpu_write, "differenceMatteOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addFloatArg(tolerance);
    kernel->addFloatArg(falloff);
  });
}