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

#include "COM_IDMaskOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
IDMaskOperation::IDMaskOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::VALUE);
  m_objectIndex = -1;
}

void IDMaskOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_objectIndex);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel idMaskOp(CCL_WRITE(dst), CCL_READ(input), const int obj_index)
{
  READ_DECL(input);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(input, dst_coords);

  READ_IMG1(input, input_pix);
  input_pix.x = (roundf(input_pix.x) == obj_index) ? 1.0f : 0.0f;
  WRITE_IMG1(dst, input_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void IDMaskOperation::execPixels(ExecutionManager &man)
{
  auto input = getInputOperation(0)->getPixels(this, man);
  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::idMaskOp, _1, input, m_objectIndex);
  computeWriteSeek(man, cpu_write, "idMaskOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input);
    kernel->addIntArg(m_objectIndex);
  });
}
