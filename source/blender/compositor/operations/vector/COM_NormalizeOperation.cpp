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

#include "COM_NormalizeOperation.h"
#include "COM_Rect.h"

#include "COM_kernel_cpu.h"

NormalizeOperation::NormalizeOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::VALUE);
  m_minv = 0.0f;
  m_maxv = 0.0f;
}

/* The code below assumes all data is inside range +- this, and that input buffer is single channel
 */
#define BLENDER_ZMAX 10000.0f
void NormalizeOperation::execPixels(ExecutionManager &man)
{
  m_minv = 1.0f + BLENDER_ZMAX;
  m_maxv = -1.0f - BLENDER_ZMAX;
  auto src = this->getInputOperation(0)->getPixels(this, man);
  auto cpu_write = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    if (ctx.current_pass == 0) {
      calcMinMultiply(src, dst);
    }
    else {
      float min_mult_x = m_minv;
      float min_mult_y = ((m_maxv != m_minv) ? 1.0f / (m_maxv - m_minv) : 0.0f);

      READ_DECL(src);
      WRITE_DECL(dst);

      CPU_LOOP_START(dst);

      COPY_COORDS(src, dst_coords);

      READ_IMG1(src, src_pix);

      src_pix.x = (src_pix.x - min_mult_x) * min_mult_y;

      /* clamp infinities */
      if (src_pix.x > 1.0f) {
        src_pix.x = 1.0f;
      }
      else if (src_pix.x < 0.0f) {
        src_pix.x = 0.0f;
      }

      WRITE_IMG1(dst, src_pix);

      CPU_LOOP_END;
    }
  };
  cpuWriteSeek(man, cpu_write);
}

void NormalizeOperation::calcMinMultiply(std::shared_ptr<PixelsRect> src, PixelsRect &dst)
{
  float minv = 1.0f + BLENDER_ZMAX;
  float maxv = -1.0f - BLENDER_ZMAX;

  READ_DECL(src);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(src, dst_coords);

  READ_IMG1(src, src_pix);

  if ((src_pix.x > maxv) && (src_pix.x <= BLENDER_ZMAX)) {
    maxv = src_pix.x;
  }
  if ((src_pix.x < minv) && (src_pix.x >= -BLENDER_ZMAX)) {
    minv = src_pix.x;
  }

  CPU_LOOP_END(dst);

  mutex.lock();
  if (maxv > m_maxv) {
    m_maxv = maxv;
  }
  if (minv < m_minv) {
    m_minv = minv;
  }
  mutex.unlock();
}