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

#include "COM_ZCombineOperation.h"
#include "BLI_utildefines.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
ZCombineOperation::ZCombineOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel zCombineOp(
    CCL_WRITE(dst), CCL_READ(color1), CCL_READ(z1), CCL_READ(color2), CCL_READ(z2))
{
  READ_DECL(color1);
  READ_DECL(z1);
  READ_DECL(color2);
  READ_DECL(z2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(color1, dst_coords, color1_pix);
  READ_IMG(z1, dst_coords, z1_pix);
  READ_IMG(color2, dst_coords, color2_pix);
  READ_IMG(z2, dst_coords, z2_pix);

  if (z1_pix.x < z2_pix.x) {
    WRITE_IMG(dst, dst_coords, color1_pix);
  }
  else {
    WRITE_IMG(dst, dst_coords, color2_pix);
  }

  CPU_LOOP_END;
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ZCombineOperation::execPixels(ExecutionManager &man)
{
  auto color1 = getInputOperation(0)->getPixels(this, man);
  auto z1 = getInputOperation(1)->getPixels(this, man);
  auto color2 = getInputOperation(2)->getPixels(this, man);
  auto z2 = getInputOperation(3)->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::zCombineOp, _1, color1, z1, color2, z2);
  computeWriteSeek(man, cpu_write, "zCombineOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*z1);
    kernel->addReadImgArgs(*color2);
    kernel->addReadImgArgs(*z2);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel zCombineAlphaOp(
    CCL_WRITE(dst), CCL_READ(color1), CCL_READ(z1), CCL_READ(color2), CCL_READ(z2))
{
  READ_DECL(color1);
  READ_DECL(z1);
  READ_DECL(color2);
  READ_DECL(z2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(z1, dst_coords, z1_pix);
  READ_IMG(z2, dst_coords, z2_pix);
  if (z1_pix.x <= z2_pix.x) {
    READ_IMG(color1, dst_coords, color1_pix);
    READ_IMG(color2, dst_coords, color2_pix);
  }
  else {
    READ_IMG(color1, dst_coords, color2_pix);
    READ_IMG(color2, dst_coords, color1_pix);
  }

  const float alpha1 = color1_pix.w;
  const float alpha2 = color2_pix.w;
  float alpha1_inv = 1.0f - alpha1;
  color2_pix = alpha1 * color1_pix + alpha1_inv * color2_pix;
  color2_pix.w = fmaxf(alpha1, alpha2);

  WRITE_IMG(dst, dst_coords, color2_pix);

  CPU_LOOP_END;
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ZCombineAlphaOperation::execPixels(ExecutionManager &man)
{
  auto color1 = getInputOperation(0)->getPixels(this, man);
  auto z1 = getInputOperation(1)->getPixels(this, man);
  auto color2 = getInputOperation(2)->getPixels(this, man);
  auto z2 = getInputOperation(3)->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::zCombineAlphaOp, _1, color1, z1, color2, z2);
  computeWriteSeek(man, cpu_write, "zCombineAlphaOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*z1);
    kernel->addReadImgArgs(*color2);
    kernel->addReadImgArgs(*z2);
  });
}

// MASK combine
ZCombineMaskOperation::ZCombineMaskOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel zCombineMaskOp(CCL_WRITE(dst), CCL_READ(mask), CCL_READ(color1), CCL_READ(color2))
{
  READ_DECL(color1);
  READ_DECL(mask);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(mask, dst_coords, mask_pix);
  READ_IMG(color1, dst_coords, color1_pix);
  READ_IMG(color2, dst_coords, color2_pix);

  color2_pix = interp_f4f4(color1_pix, color2_pix, 1.0f - mask_pix.x);

  WRITE_IMG(dst, dst_coords, color2_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ZCombineMaskOperation::execPixels(ExecutionManager &man)
{
  auto mask = getInputOperation(0)->getPixels(this, man);
  auto color1 = getInputOperation(1)->getPixels(this, man);
  auto color2 = getInputOperation(2)->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::zCombineMaskOp, _1, mask, color1, color2);
  computeWriteSeek(man, cpu_write, "zCombineMaskOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*mask);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel zCombineMaskAlphaOp(CCL_WRITE(dst), CCL_READ(mask), CCL_READ(color1), CCL_READ(color2))
{
  READ_DECL(color1);
  READ_DECL(mask);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(mask, dst_coords, mask_pix);
  READ_IMG(color1, dst_coords, color1_pix);
  READ_IMG(color2, dst_coords, color2_pix);

  const float alpha1 = color1_pix.w;
  const float alpha2 = color2_pix.w;
  const float fac = (1.0f - mask_pix.x) * (1.0f - alpha1) + mask_pix.x * alpha2;
  const float mfac = 1.0f - fac;
  color2_pix = mfac * color1_pix + fac * color2_pix;
  color2_pix.w = fmaxf(alpha1, alpha2);

  WRITE_IMG(dst, dst_coords, color2_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void ZCombineMaskAlphaOperation::execPixels(ExecutionManager &man)
{
  auto mask = getInputOperation(0)->getPixels(this, man);
  auto color1 = getInputOperation(1)->getPixels(this, man);
  auto color2 = getInputOperation(2)->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::zCombineMaskAlphaOp, _1, mask, color1, color2);
  computeWriteSeek(man, cpu_write, "zCombineMaskAlphaOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*mask);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
  });
}
