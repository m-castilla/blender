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

#include "COM_MapRangeOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
MapRangeOperation::MapRangeOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::VALUE);
  this->m_use_clamp = false;
}

void MapRangeOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_use_clamp);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

/* The code below assumes all data is inside range +- this, and that input buffer is single channel
 */
#define BLENDER_ZMAX 10000.0f

ccl_kernel mapRangeOp(CCL_WRITE(dst),
                      CCL_READ(input),
                      CCL_READ(src_min),
                      CCL_READ(src_max),
                      CCL_READ(dst_min),
                      CCL_READ(dst_max),
                      BOOL use_clamp)
{
  READ_DECL(input);
  READ_DECL(src_min);
  READ_DECL(src_max);
  READ_DECL(dst_min);
  READ_DECL(dst_max);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(src_min, dst_coords);
  COPY_COORDS(src_max, dst_coords);

  READ_IMG1(src_min, src_min_pix);
  READ_IMG1(src_max, src_max_pix);

  if (fabsf(src_max_pix.x - src_min_pix.x) < 1e-6f) {
    input_pix.x = 0.0f;
    WRITE_IMG1(dst, input_pix);
  }
  else {
    COPY_COORDS(input, dst_coords);
    COPY_COORDS(dst_min, dst_coords);
    COPY_COORDS(dst_max, dst_coords);

    READ_IMG1(input, input_pix);
    READ_IMG1(dst_min, dst_min_pix);
    READ_IMG1(dst_max, dst_max_pix);
    if (input_pix.x >= -BLENDER_ZMAX && input_pix.x <= BLENDER_ZMAX) {
      input_pix.x = (input_pix.x - src_min_pix.x) / (src_max_pix.x - src_min_pix.x);
      input_pix.x = dst_min_pix.x + input_pix.x * (dst_max_pix.x - dst_min_pix.x);
    }
    else if (input_pix.x > BLENDER_ZMAX) {
      input_pix.x = dst_max_pix.x;
    }
    else {
      input_pix.x = dst_min_pix.x;
    }

    if (use_clamp) {
      if (dst_max_pix.x > dst_min_pix.x) {
        input_pix.x = clamp(input_pix.x, dst_min_pix.x, dst_max_pix.x);
      }
      else {
        input_pix.x = clamp(input_pix.x, dst_max_pix.x, dst_max_pix.x);
      }
    }

    WRITE_IMG1(dst, input_pix);
  }

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void MapRangeOperation::execPixels(ExecutionManager &man)
{
  auto input = getInputOperation(0)->getPixels(this, man);
  auto src_min = getInputOperation(1)->getPixels(this, man);
  auto src_max = getInputOperation(2)->getPixels(this, man);
  auto dst_min = getInputOperation(3)->getPixels(this, man);
  auto dst_max = getInputOperation(4)->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::mapRangeOp, _1, input, src_min, src_max, dst_min, dst_max, m_use_clamp);
  computeWriteSeek(man, cpu_write, "mapRangeOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input);
    kernel->addReadImgArgs(*src_min);
    kernel->addReadImgArgs(*src_max);
    kernel->addReadImgArgs(*dst_min);
    kernel->addReadImgArgs(*dst_max);
    kernel->addBoolArg(m_use_clamp);
  });
}