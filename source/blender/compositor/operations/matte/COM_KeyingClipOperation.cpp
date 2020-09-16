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

#include "COM_KeyingClipOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
KeyingClipOperation::KeyingClipOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::VALUE);

  this->m_kernelRadius = 3;
  this->m_kernelTolerance = 0.1f;

  this->m_clipBlack = 0.0f;
  this->m_clipWhite = 1.0f;

  this->m_isEdgeMatte = false;
}

void KeyingClipOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_clipBlack);
  hashParam(m_clipWhite);
  hashParam(m_kernelRadius);
  hashParam(m_kernelTolerance);
  hashParam(m_isEdgeMatte);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel keyingClipOp(CCL_WRITE(dst),
                        CCL_READ(value),
                        int delta,
                        int w,
                        int h,
                        float tolerance,
                        float black_clip,
                        float white_clip,
                        BOOL is_edge_matte)
{
  READ_DECL(value);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);

  READ_IMG1(value, value_pix);
  float start_value = value_pix.x;

  bool ok = false;
  int start_x = fmaxf(0, dst_coords.x - delta + 1), start_y = fmaxf(0, dst_coords.y - delta + 1),
      end_x = fminf(dst_coords.x + delta - 1, w - 1),
      end_y = fminf(dst_coords.y + delta - 1, h - 1);

  int count = 0, totalCount = (end_x - start_x + 1) * (end_y - start_y + 1) - 1;
  int thresholdCount = ceilf((float)totalCount * 0.9f);

  if (delta == 0) {
    ok = true;
  }

  SET_COORDS(value, start_x, start_y);
  while (value_coords.x <= end_x && ok == false) {
    while (value_coords.y <= end_y && ok == false) {
      if (UNLIKELY(value_coords.x == dst_coords.x && value_coords.y == dst_coords.y)) {
        INCR1_COORDS_Y(value);
        continue;
      }

      READ_IMG1(value, value_pix);

      if (fabsf(value_pix.x - start_value) < tolerance) {
        count++;
        if (count >= thresholdCount) {
          ok = true;
        }
      }
      INCR1_COORDS_Y(value);
    }
    INCR1_COORDS_X(value);
    UPDATE_COORDS_Y(value, start_y);
  }
  // for (int cx = start_x; ok == false && cx <= end_x; cx++) {
  //  for (int cy = start_y; ok == false && cy <= end_y; cy++) {
  //    if (UNLIKELY(cx == dst_coords.x && cy == dst_coords.y)) {
  //      continue;
  //    }

  //    READ_IMG1(value, value_pix);

  //    if (fabsf(value_pix.x - start_value) < tolerance) {
  //      count++;
  //      if (count >= thresholdCount) {
  //        ok = true;
  //      }
  //    }
  //  }
  //}

  if (is_edge_matte) {
    if (ok) {
      value_pix.x = 0.0f;
    }
    else {
      value_pix.x = 1.0f;
    }
  }
  else {
    if (ok) {
      if (start_value < black_clip) {
        value_pix.x = 0.0f;
      }
      else if (start_value >= white_clip) {
        value_pix.x = 1.0f;
      }
      else {
        value_pix.x = (start_value - black_clip) / (white_clip - black_clip);
      }
    }
    else {
      value_pix.x = start_value;
    }
  }

  WRITE_IMG1(dst, value_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void KeyingClipOperation::execPixels(ExecutionManager &man)
{
  auto value = getInputOperation(0)->getPixels(this, man);
  const int delta = this->m_kernelRadius;
  const float tolerance = this->m_kernelTolerance;
  const int w = getWidth();
  const int h = getHeight();
  auto cpu_write = std::bind(CCL::keyingClipOp,
                             _1,
                             value,
                             delta,
                             w,
                             h,
                             tolerance,
                             m_clipBlack,
                             m_clipWhite,
                             m_isEdgeMatte);
  computeWriteSeek(man, cpu_write, "keyingClipOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addIntArg(delta);
    kernel->addIntArg(w);
    kernel->addIntArg(h);
    kernel->addFloatArg(tolerance);
    kernel->addFloatArg(m_clipBlack);
    kernel->addFloatArg(m_clipWhite);
    kernel->addBoolArg(m_isEdgeMatte);
  });
}
