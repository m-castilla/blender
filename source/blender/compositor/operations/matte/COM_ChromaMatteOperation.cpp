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

#include "COM_ChromaMatteOperation.h"
#include "COM_ComputeKernel.h"

#include "COM_kernel_cpu.h"

using namespace std::placeholders;

ChromaMatteOperation::ChromaMatteOperation() : NodeOperation()
{
  addInputSocket(SocketType::COLOR);
  addInputSocket(SocketType::COLOR);
  addOutputSocket(SocketType::VALUE);
}
void ChromaMatteOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_settings->t1);
  hashParam(m_settings->t2);
  hashParam(m_settings->fstrength);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel chromaMatteOp(CCL_WRITE(dst),
                         CCL_READ(color),
                         CCL_READ(key),
                         const float acceptance,
                         const float cutoff,
                         const float gain)
{
  READ_DECL(color);
  READ_DECL(key);
  WRITE_DECL(dst);
  float x_angle, z_angle, alpha;
  float theta, beta;
  float kfg;

  CPU_LOOP_START(dst);

  COPY_COORDS(color, dst_coords);
  COPY_COORDS(key, dst_coords);

  READ_IMG4(color, color_pix);
  READ_IMG4(key, key_pix);

  /* store matte(alpha) value in [0] to go with
   * COM_SetAlphaOperation and the Value output
   */

  /* Algorithm from book "Video Demistified," does not include the spill reduction part */
  /* find theta, the angle that the color space should be rotated based on key */

  /* rescale to -1.0..1.0 */
  // color_pix.x = (color_pix.x  * 2.0f) - 1.0f;  // UNUSED
  color_pix.y = (color_pix.y * 2.0f) - 1.0f;
  color_pix.z = (color_pix.z * 2.0f) - 1.0f;

  // key_pix.x = (key_pix.x * 2.0f) - 1.0f;  // UNUSED
  key_pix.y = (key_pix.y * 2.0f) - 1.0f;
  key_pix.z = (key_pix.z * 2.0f) - 1.0f;

  theta = atan2(key_pix.z, key_pix.y);

  /*rotate the cb and cr into x/z space */
  x_angle = color_pix.y * cosf(theta) + color_pix.z * sinf(theta);
  z_angle = color_pix.z * cosf(theta) - color_pix.y * sinf(theta);

  /*if within the acceptance angle */
  /* if kfg is <0 then the pixel is outside of the key color */
  kfg = x_angle - (fabsf(z_angle) / tanf(acceptance / 2.0f));

  if (kfg > 0.0f) { /* found a pixel that is within key color */
    alpha = (gain == 0.0f) ? 0.0f : (1.0f - (kfg / gain));

    beta = atan2(z_angle, x_angle);

    /* if beta is within the cutoff angle */
    if (fabsf(beta) < (cutoff / 2.0f)) {
      alpha = 0.0f;
    }

    /* don't make something that was more transparent less transparent */
    if (alpha < color_pix.w) {
      color_pix.x = alpha;
    }
    else {
      color_pix.x = color_pix.w;
    }
  }
  else {                       /*pixel is outside key color */
    color_pix.x = color_pix.w; /* make pixel just as transparent as it was before */
  }

  WRITE_IMG1(dst, color_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void ChromaMatteOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto key = getInputOperation(1)->getPixels(this, man);
  const float acceptance = this->m_settings->t1; /* in radians */
  const float cutoff = this->m_settings->t2;     /* in radians */
  const float gain = this->m_settings->fstrength;
  auto cpu_write = std::bind(CCL::chromaMatteOp, _1, color, key, acceptance, cutoff, gain);
  computeWriteSeek(man, cpu_write, "chromaMatteOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*key);
    kernel->addFloatArg(acceptance);
    kernel->addFloatArg(cutoff);
    kernel->addFloatArg(gain);
  });
}
