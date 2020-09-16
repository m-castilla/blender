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
 * Copyright 2012, Blender Foundation.
 */

#include "COM_ChannelMatteOperation.h"
#include "BLI_math.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
ChannelMatteOperation::ChannelMatteOperation() : NodeOperation()
{
  addInputSocket(SocketType::COLOR);
  addOutputSocket(SocketType::VALUE);
}

void ChannelMatteOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_limit_max);
  hashParam(m_limit_min);
  hashParam(m_limit_method);
  hashParam(m_limit_channel);
  hashParam(m_matte_channel);
}

void ChannelMatteOperation::initExecution()
{
  NodeOperation::initExecution();
  switch (m_limit_method) {
    /* SINGLE */
    case 0: {
      /* 123 / RGB / HSV / YUV / YCC */
      const int matte_channel = m_matte_channel - 1;
      const int limit_channel = m_limit_channel - 1;
      this->m_ids[0] = matte_channel;
      this->m_ids[1] = limit_channel;
      this->m_ids[2] = limit_channel;
      break;
    }
    /* MAX */
    case 1: {
      switch (m_matte_channel) {
        case 1: {
          this->m_ids[0] = 0;
          this->m_ids[1] = 1;
          this->m_ids[2] = 2;
          break;
        }
        case 2: {
          this->m_ids[0] = 1;
          this->m_ids[1] = 0;
          this->m_ids[2] = 2;
          break;
        }
        case 3: {
          this->m_ids[0] = 2;
          this->m_ids[1] = 0;
          this->m_ids[2] = 1;
          break;
        }
        default:
          break;
      }
      break;
    }
    default:
      break;
  }
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel channelMatteOp(CCL_WRITE(dst),
                          CCL_READ(color),
                          float limit_min,
                          float limit_max,
                          float limit_range,
                          int id0,
                          int id1,
                          int id2)
{
  float *rgb;
  float alpha;

  READ_DECL(color);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(color, dst_coords);
  READ_IMG4(color, color_pix);

  /* matte operation */
  rgb = (float *)&color_pix;
  alpha = rgb[id0] - max(rgb[id1], rgb[id2]);

  /* flip because 0.0 is transparent, not 1.0 */
  alpha = 1.0f - alpha;

  /* test range */
  if (alpha > limit_max) {
    alpha = color_pix.w; /*whatever it was prior */
  }
  else if (alpha < limit_min) {
    alpha = 0.0f;
  }
  else { /*blend */
    if (limit_range == 0.0f) {
      alpha = color_pix.w;
    }
    else {
      alpha = (alpha - limit_min) / limit_range;
    }
  }

  /* store matte(alpha) value in [0] to go with
   * COM_SetAlphaOperation and the Value output
   */

  /* don't make something that was more transparent less transparent */
  color_pix.x = min(alpha, color_pix.w);
  WRITE_IMG1(dst, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ChannelMatteOperation::execPixels(ExecutionManager &man)
{
  const float limit_range = m_limit_max - m_limit_min;
  auto color = getInputOperation(0)->getPixels(this, man);
  auto cpu_write = std::bind(CCL::channelMatteOp,
                             _1,
                             color,
                             m_limit_min,
                             m_limit_max,
                             limit_range,
                             m_ids[0],
                             m_ids[1],
                             m_ids[2]);
  computeWriteSeek(man, cpu_write, "channelMatteOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addFloatArg(m_limit_min);
    kernel->addFloatArg(m_limit_max);
    kernel->addFloatArg(limit_range);
    kernel->addIntArg(m_ids[0]);
    kernel->addIntArg(m_ids[1]);
    kernel->addIntArg(m_ids[2]);
  });
}
