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

#include "COM_DotProductOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
DotproductOperation::DotproductOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VECTOR);
  this->addInputSocket(SocketType::VECTOR);
  this->addOutputSocket(SocketType::VALUE);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel dotProductOp(CCL_WRITE(dst), CCL_READ(vector1), CCL_READ(vector2))
{
  READ_DECL(vector1);
  READ_DECL(vector2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(vector1, dst_coords);
  COPY_COORDS(vector2, dst_coords);
  READ_IMG3(vector1, vector1_pix);
  READ_IMG3(vector2, vector2_pix);

  vector1_pix.x = -(vector1_pix.x * vector2_pix.x + vector1_pix.y * vector2_pix.y +
                    vector1_pix.z * vector2_pix.z);

  WRITE_IMG1(dst, vector1_pix);

  CPU_LOOP_END;
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void DotproductOperation::execPixels(ExecutionManager &man)
{
  auto vector1 = getInputOperation(0)->getPixels(this, man);
  auto vector2 = getInputOperation(1)->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::dotProductOp, _1, vector1, vector2);
  computeWriteSeek(man, cpu_write, "dotProductOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*vector1);
    kernel->addReadImgArgs(*vector2);
  });
}
