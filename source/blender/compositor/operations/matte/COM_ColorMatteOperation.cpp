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

#include "COM_ColorMatteOperation.h"
#include "COM_ChromaMatteOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;

ColorMatteOperation::ColorMatteOperation() : NodeOperation()
{
  addInputSocket(SocketType::COLOR);
  addInputSocket(SocketType::COLOR);
  addOutputSocket(SocketType::VALUE);
}
void ColorMatteOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_settings->t1);
  hashParam(m_settings->t2);
  hashParam(m_settings->t3);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel colorMatteOp(CCL_WRITE(dst),
                        CCL_READ(color),
                        CCL_READ(key),
                        const float hue,
                        const float sat,
                        const float val)
{
  READ_DECL(color);
  READ_DECL(key);
  WRITE_DECL(dst);
  float h_wrap;

  CPU_LOOP_START(dst);

  COPY_COORDS(color, dst_coords);
  COPY_COORDS(key, dst_coords);

  READ_IMG4(color, color_pix);
  READ_IMG4(key, key_pix);
  /* store matte(alpha) value in [0] to go with
   * COM_SetAlphaOperation and the Value output
   */

  if (
      /* do hue last because it needs to wrap, and does some more checks  */

      /* sat */ (fabsf(color_pix.y - key_pix.y) < sat) &&
      /* val */ (fabsf(color_pix.z - key_pix.z) < val) &&

      /* multiply by 2 because it wraps on both sides of the hue,
       * otherwise 0.5 would key all hue's */

      /* hue */
      ((h_wrap = 2.0f * fabsf(color_pix.x - key_pix.x)) < hue || (2.0f - h_wrap) < hue)) {
    color_pix.x = 0.0f; /* make transparent */
  }

  else {                       /*pixel is outside key color */
    color_pix.x = color_pix.w; /* make pixel just as transparent as it was before */
  }

  WRITE_IMG1(dst, color_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void ColorMatteOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto key = getInputOperation(1)->getPixels(this, man);
  const float hue = this->m_settings->t1;
  const float sat = this->m_settings->t2;
  const float val = this->m_settings->t3;
  auto cpu_write = std::bind(CCL::colorMatteOp, _1, color, key, hue, sat, val);
  computeWriteSeek(man, cpu_write, "colorMatteOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*key);
    kernel->addFloatArg(hue);
    kernel->addFloatArg(sat);
    kernel->addFloatArg(val);
  });
}