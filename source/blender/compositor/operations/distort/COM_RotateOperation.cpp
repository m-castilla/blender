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

#include "COM_RotateOperation.h"
#include "BLI_math.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_GlobalManager.h"

#include "COM_kernel_cpu.h"

RotateOperation::RotateOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::DYNAMIC);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::DYNAMIC);
  m_sampler = GlobalMan->getContext()->getDefaultSampler();
}

void RotateOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_sampler);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel rotateOp(CCL_WRITE(dst),
                    CCL_READ(input),
                    CCL_SAMPLER(sampler),
                    const float rad_cosine,
                    const float rad_sine,
                    const float center_x,
                    const float center_y)
{
  READ_DECL(input);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  const float dx = dst_coords.x - center_x;
  const float dy = dst_coords.y - center_y;
  const float nx = center_x + (rad_cosine * dx + rad_sine * dy);
  const float ny = center_y + (-rad_sine * dx + rad_cosine * dy);

  SET_SAMPLE_COORDS(input, nx, ny);
  SAMPLE_IMG(input, sampler, input_pix);
  WRITE_IMG(dst, input_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

using namespace std::placeholders;
void RotateOperation::execPixels(ExecutionManager &man)
{
  auto input = getInputOperation(0)->getPixels(this, man);
  auto radians_pix = getInputOperation(1)->getSinglePixel(this, man, 0, 0);
  float radians = 0.0f;
  if (radians_pix != nullptr) {
    radians = radians_pix[0];
  }

  const int width = getWidth();
  const int height = getHeight();
  const float center_x = (width - 1) / 2.0;
  const float center_y = (height - 1) / 2.0;
  const float cosine = cos(radians);
  const float sine = sin(radians);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::rotateOp, _1, input, m_sampler, cosine, sine, center_x, center_y);
  computeWriteSeek(man, cpu_write, "rotateOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input);
    kernel->addSamplerArg(m_sampler);
    kernel->addFloatArg(cosine);
    kernel->addFloatArg(sine);
    kernel->addFloatArg(center_x);
    kernel->addFloatArg(center_y);
  });
}