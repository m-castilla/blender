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

#include "COM_GaussianAlphaYBlurOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_kernel_cpu.h"
#include "MEM_guardedalloc.h"

GaussianAlphaYBlurOperation::GaussianAlphaYBlurOperation()
    : GaussianBlurBaseOperation(SocketType::VALUE)
{
}

void GaussianAlphaYBlurOperation::initExecution()
{
  /* BlurBaseOperation::initExecution(); */ /* until we support size input - comment this */
  NodeOperation::initExecution();
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel gaussianAlphaYBlurOp(CCL_WRITE(dst),
                                CCL_READ(input),
                                ccl_constant float *gausstab,
                                ccl_constant float *distbuf_inv,
                                BOOL do_invert,
                                int filter_size,
                                int height,
                                int quality_step)
{
  READ_DECL(input);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  int ymin = max(dst_coords.y - filter_size, 0);
  int ymax = min(dst_coords.y + filter_size + 1, height);

  /* *** this is the main part which is different to 'GaussianXBlurOperation'  *** */

  /* gauss */
  float alpha_accum = 0.0f;
  float multiplier_accum = 0.0f;

  /* dilate */
  COPY_COORDS(input, dst_coords);
  READ_IMG1(input, input_pix);
  float value_max = finv_test(
      input_pix.x, do_invert); /* init with the current color to avoid unneeded lookups */
  float distfacinv_max = 1.0f; /* 0 to 1 */

  UPDATE_COORDS_Y(input, ymin);
  while (input_coords.y < ymax) {
    const int index = (input_coords.y - dst_coords.y) + filter_size;
    READ_IMG1(input, input_pix);
    float value = finv_test(input_pix.x, do_invert);
    float multiplier;

    /* gauss */
    {
      multiplier = gausstab[index];
      alpha_accum += value * multiplier;
      multiplier_accum += multiplier;
    }

    /* dilate - find most extreme color */
    if (value > value_max) {
      multiplier = distbuf_inv[index];
      value *= multiplier;
      if (value > value_max) {
        value_max = value;
        distfacinv_max = multiplier;
      }
    }
    INCR_COORDS_Y(input, quality_step);
  }

  /* blend between the max value and gauss blue - gives nice feather */
  const float value_blur = alpha_accum / multiplier_accum;
  const float value_final = (value_max * distfacinv_max) + (value_blur * (1.0f - distfacinv_max));
  input_pix.x = finv_test(value_final, do_invert);

  WRITE_IMG1(dst, input_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void GaussianAlphaYBlurOperation::execPixels(ExecutionManager &man)
{
  execGaussianPixels(man, false, true, CCL::gaussianAlphaYBlurOp, "gaussianAlphaYBlurOp");
}

void GaussianAlphaYBlurOperation::deinitExecution()
{
  /* BlurBaseOperation::initExecution(); */ /* until we support size input - comment this */
  NodeOperation::deinitExecution();
}
