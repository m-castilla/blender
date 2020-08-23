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

#include "COM_GaussianYBlurOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_GlobalManager.h"
#include "COM_kernel_cpu.h"
#include "MEM_guardedalloc.h"

GaussianYBlurOperation::GaussianYBlurOperation() : GaussianBlurBaseOperation(SocketType::COLOR)
{
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel gaussianYBlurOp(CCL_WRITE(dst),
                           CCL_READ(color),
                           ccl_constant float *gausstab,
                           ccl_constant float *UNUSED(distbuf_inv),
                           BOOL UNUSED(do_invert),
                           int filter_size,
                           int height,
                           int quality_step)
{
  READ_DECL(color);
  WRITE_DECL(dst);
  float4 color_accum;
  float4 multiplier;
  float gauss_value;
  float multiplier_accum;

  CPU_LOOP_START(dst);

  int ymin = max(dst_coords.y - filter_size, 0);
  int ymax = min(dst_coords.y + filter_size + 1, height);

  /* *** this is the main part which is different to 'GaussianXBlurOperation'  *** */

  /* gauss */
  color_accum = make_float4_1(0.0f);
  multiplier_accum = 0.0f;

  SET_COORDS(color, dst_coords.x, ymin);
  for (int ny = ymin, index = (ymin - dst_coords.y) + filter_size; ny < ymax;
       ny += quality_step, index += quality_step) {
    gauss_value = gausstab[index];
    READ_IMG4(color, color_pix);
    multiplier = make_float4_1(gauss_value);
    color_accum += color_pix * multiplier;
    multiplier_accum += gauss_value;
    INCR1_COORDS_Y(color);
  }

  color_pix = color_accum * (1.0f / multiplier_accum);

  WRITE_IMG4(dst, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void GaussianYBlurOperation::execPixels(ExecutionManager &man)
{
  execGaussianPixels(man, false, false, CCL::gaussianYBlurOp, "gaussianYBlurOp");
}
