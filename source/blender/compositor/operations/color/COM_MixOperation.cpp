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

#include "COM_MixOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_PixelsUtil.h"
#include "COM_Rect.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
MixBaseOperation::MixBaseOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
  this->m_input_value = NULL;
  this->m_input_color1 = NULL;
  this->m_input_color2 = NULL;
  this->setUseValueAlphaMultiply(false);
  this->setUseClamp(false);

  this->setMainInputSocketIndex(1);
}

void MixBaseOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(this->m_valueAlphaMultiply);
  hashParam(this->m_useClamp);
}

void MixBaseOperation::initExecution()
{
  this->m_input_value = this->getInputOperation(0);
  this->m_input_color1 = this->getInputOperation(1);
  this->m_input_color2 = this->getInputOperation(2);
  NodeOperation::initExecution();
}

// void MixBaseOperation::determineResolution(int resolution[2], int preferredResolution[2])
//{
//  NodeOperationInput *socket;
//  int tempPreferredResolution[2] = {0, 0};
//  int tempResolution[2];
//
//  socket = this->getInputSocket(1);
//  socket->determineResolution(tempResolution, tempPreferredResolution);
//  if ((tempResolution[0] != 0) && (tempResolution[1] != 0)) {
//    this->setMainInputSocketIndex(1);
//  }
//  else {
//    socket = this->getInputSocket(2);
//    socket->determineResolution(tempResolution, tempPreferredResolution);
//    if ((tempResolution[0] != 0) && (tempResolution[1] != 0)) {
//      this->setMainInputSocketIndex(2);
//    }
//    else {
//      this->setMainInputSocketIndex(0);
//    }
//  }
//  NodeOperation::determineResolution(resolution, preferredResolution);
//}

void MixBaseOperation::deinitExecution()
{
  this->m_input_value = NULL;
  this->m_input_color1 = NULL;
  this->m_input_color2 = NULL;
  NodeOperation::deinitExecution();
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel mixBaseOp(
    CCL_WRITE(dst), CCL_READ(value), CCL_READ(color1), CCL_READ(color2), BOOL alpha_multiply)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }
  color2_pix = (1.0f - value_pix.x) * color1_pix + value_pix.x * color2_pix;
  color2_pix.w = color1_pix.w;

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixBaseOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(CCL::mixBaseOp, _1, value, color1, color2, m_valueAlphaMultiply);
  computeWriteSeek(man, cpu_write, "mixBaseOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel mixAddOp(CCL_WRITE(dst),
                    CCL_READ(value),
                    CCL_READ(color1),
                    CCL_READ(color2),
                    BOOL alpha_multiply,
                    BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }
  color2_pix = color1_pix + value_pix.x * color2_pix;
  color2_pix.w = color1_pix.w;
  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixAddOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixAddOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixAddOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel mixBlendOp(CCL_WRITE(dst),
                      CCL_READ(value),
                      CCL_READ(color1),
                      CCL_READ(color2),
                      BOOL alpha_multiply,
                      BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }
  color2_pix = (1.0f - value_pix.x) * color1_pix + value_pix.x * color2_pix;
  color2_pix.w = color1_pix.w;
  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixBlendOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixBlendOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixBlendOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel mixColorBurnOp(
    CCL_WRITE(dst), CCL_READ(value), CCL_READ(color1), CCL_READ(color2), BOOL alpha_multiply)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }

  float4 zero4 = make_float4_1(0.0f);
  color2_pix = (1.0f - value_pix.x) + value_pix.x * color2_pix;
  int4 color2_eqless_0 = color2_pix <= zero4;
  color2_pix = 1.0f - (1.0f - color1_pix) / color2_pix;
  color2_pix = select(color2_pix, zero4, color2_eqless_0);
  color2_pix.w = color1_pix.w;
  color2_pix = clamp_to_normal_f4(color2_pix);

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixColorBurnOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(CCL::mixColorBurnOp, _1, value, color1, color2, m_valueAlphaMultiply);
  computeWriteSeek(man, cpu_write, "mixColorBurnOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel mixColorOp(CCL_WRITE(dst),
                      CCL_READ(value),
                      CCL_READ(color1),
                      CCL_READ(color2),
                      BOOL alpha_multiply,
                      BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }

  float4 color2_hsv = rgb_to_hsv(color2_pix);
  if (color2_hsv.y != 0.0f) {
    float4 color1_hsv = rgb_to_hsv(color1_pix);
    color2_hsv.z = color1_hsv.z;
    color2_pix = hsv_to_rgb(color2_hsv);
    color2_pix = (1.0f - value_pix.x) * color1_pix + value_pix.x * color2_pix;
    color2_pix.w = color1_pix.w;
  }
  else {
    color2_pix = color1_pix;
  }

  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixColorOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixColorOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixColorOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mixDarkenOp(CCL_WRITE(dst),
                       CCL_READ(value),
                       CCL_READ(color1),
                       CCL_READ(color2),
                       BOOL alpha_multiply,
                       BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }

  color2_pix = min(color1_pix, color2_pix) * value_pix.x + color1_pix * (1.0f - value_pix.x);
  color2_pix.w = color1_pix.w;
  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixDarkenOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixDarkenOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixDarkenOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mixDifferenceOp(CCL_WRITE(dst),
                           CCL_READ(value),
                           CCL_READ(color1),
                           CCL_READ(color2),
                           BOOL alpha_multiply,
                           BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }

  color2_pix = (1.0f - value_pix.x) * color1_pix + value_pix.x * fabs(color1_pix - color2_pix);
  color2_pix.w = color1_pix.w;
  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixDifferenceOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixDifferenceOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixDifferenceOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mixDivideOp(CCL_WRITE(dst),
                       CCL_READ(value),
                       CCL_READ(color1),
                       CCL_READ(color2),
                       BOOL alpha_multiply,
                       BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }

  float4 zero4 = make_float4_1(0.0f);
  int4 color2_not0 = color2_pix != zero4;
  color2_pix = (1.0f - value_pix.x) * color1_pix + value_pix.x * color1_pix / color2_pix;
  color2_pix = select(zero4, color2_pix, color2_not0);
  color2_pix.w = color1_pix.w;
  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixDivideOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixDivideOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixDivideOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mixDodgeOp(CCL_WRITE(dst),
                      CCL_READ(value),
                      CCL_READ(color1),
                      CCL_READ(color2),
                      BOOL alpha_multiply,
                      BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }

  if (color1_pix.x != 0.0f) {
    color2_pix.x = 1.0f - value_pix.x * color2_pix.x;
    if (color2_pix.x <= 0.0f) {
      color2_pix.x = 1.0f;
    }
    else {
      color2_pix.x = color1_pix.x / color2_pix.x;
      if (color2_pix.x > 1.0f) {
        color2_pix.x = 1.0f;
      }
    }
  }
  else {
    color2_pix.x = 0.0f;
  }

  if (color1_pix.y != 0.0f) {
    color2_pix.y = 1.0f - value_pix.x * color2_pix.y;
    if (color2_pix.y <= 0.0f) {
      color2_pix.y = 1.0f;
    }
    else {
      color2_pix.y = color1_pix.y / color2_pix.y;
      if (color2_pix.y > 1.0f) {
        color2_pix.y = 1.0f;
      }
    }
  }
  else {
    color2_pix.y = 0.0f;
  }

  if (color1_pix.z != 0.0f) {
    color2_pix.z = 1.0f - value_pix.x * color2_pix.z;
    if (color2_pix.z <= 0.0f) {
      color2_pix.z = 1.0f;
    }
    else {
      color2_pix.z = color1_pix.z / color2_pix.z;
      if (color2_pix.z > 1.0f) {
        color2_pix.z = 1.0f;
      }
    }
  }
  else {
    color2_pix.z = 0.0f;
  }

  color2_pix.w = color1_pix.w;
  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixDodgeOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixDodgeOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixDodgeOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mixGlareOp(
    CCL_WRITE(dst), CCL_READ(value), CCL_READ(color1), CCL_READ(color2), BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  float mf = 2.0f - 2.0f * fabsf(value_pix.x - 0.5f);
  float4 zero4 = make_float4_1(0.0f);
  float alpha = color1_pix.w;
  color1_pix = select(color1_pix, zero4, color1_pix < zero4);
  // if (color1_pix.x < 0.0f) {
  //  color1_pix.x = 0.0f;
  //}
  // if (color1_pix.y < 0.0f) {
  //  color1_pix.y = 0.0f;
  //}
  // if (color1_pix.z < 0.0f) {
  //  color1_pix.z = 0.0f;
  //}
  color2_pix = mf * max(color1_pix + value_pix.x * (color2_pix - color1_pix), 0.0f);
  color2_pix.w = alpha;
  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixGlareOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(CCL::mixGlareOp, _1, value, color1, color2, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixGlareOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel mixHueOp(CCL_WRITE(dst),
                    CCL_READ(value),
                    CCL_READ(color1),
                    CCL_READ(color2),
                    BOOL alpha_multiply,
                    BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }

  float4 color2_hsv = rgb_to_hsv(color2_pix);
  if (color2_hsv.y != 0.0f) {
    float4 color1_hsv = rgb_to_hsv(color1_pix);
    color1_hsv.x = color2_hsv.x;
    color2_pix = hsv_to_rgb(color1_hsv);
    color2_pix = (1.0f - value_pix.x) * color1_pix + value_pix.x * color2_pix;
    color2_pix.w = color1_pix.w;
  }
  else {
    color2_pix = color1_pix;
  }

  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixHueOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixHueOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixHueOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mixLightenOp(CCL_WRITE(dst),
                        CCL_READ(value),
                        CCL_READ(color1),
                        CCL_READ(color2),
                        BOOL alpha_multiply,
                        BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }

  color2_pix = value_pix.x * color2_pix;
  color2_pix = select(color1_pix, color2_pix, color2_pix > color1_pix);
  color2_pix.w = color1_pix.w;
  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixLightenOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixLightenOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixLightenOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mixLinearLightOp(CCL_WRITE(dst),
                            CCL_READ(value),
                            CCL_READ(color1),
                            CCL_READ(color2),
                            BOOL alpha_multiply,
                            BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }

  if (color2_pix.x > 0.5f) {
    color2_pix.x = color1_pix.x + value_pix.x * (2.0f * (color2_pix.x - 0.5f));
  }
  else {
    color2_pix.x = color1_pix.x + value_pix.x * (2.0f * color2_pix.x - 1.0f);
  }
  if (color2_pix.y > 0.5f) {
    color2_pix.y = color1_pix.y + value_pix.x * (2.0f * (color2_pix.y - 0.5f));
  }
  else {
    color2_pix.y = color1_pix.y + value_pix.x * (2.0f * color2_pix.y - 1.0f);
  }
  if (color2_pix.z > 0.5f) {
    color2_pix.z = color1_pix.z + value_pix.x * (2.0f * (color2_pix.z - 0.5f));
  }
  else {
    color2_pix.z = color1_pix.z + value_pix.x * (2.0f * color2_pix.z - 1.0f);
  }
  color2_pix.w = color1_pix.w;

  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixLinearLightOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixLinearLightOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixLinearLightOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mixMultiplyOp(CCL_WRITE(dst),
                         CCL_READ(value),
                         CCL_READ(color1),
                         CCL_READ(color2),
                         BOOL alpha_multiply,
                         BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }

  color2_pix = color1_pix * ((1.0f - value_pix.x) + value_pix.x * color2_pix);
  color2_pix.w = color1_pix.w;
  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixMultiplyOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixMultiplyOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixMultiplyOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mixOverlayOp(CCL_WRITE(dst),
                        CCL_READ(value),
                        CCL_READ(color1),
                        CCL_READ(color2),
                        BOOL alpha_multiply,
                        BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }

  float valuem = 1.0f - value_pix.x;
  float value2 = 2.0f * value_pix.x;
  if (color1_pix.x < 0.5f) {
    color2_pix.x = color1_pix.x * (valuem + value2 * color2_pix.x);
  }
  else {
    color2_pix.x = 1.0f - (valuem + value2 * (1.0f - color2_pix.x)) * (1.0f - color1_pix.x);
  }
  if (color1_pix.y < 0.5f) {
    color2_pix.y = color1_pix.y * (valuem + value2 * color2_pix.y);
  }
  else {
    color2_pix.y = 1.0f - (valuem + value2 * (1.0f - color2_pix.y)) * (1.0f - color1_pix.y);
  }
  if (color1_pix.z < 0.5f) {
    color2_pix.z = color1_pix.z * (valuem + value2 * color2_pix.z);
  }
  else {
    color2_pix.z = 1.0f - (valuem + value2 * (1.0f - color2_pix.z)) * (1.0f - color1_pix.z);
  }
  color2_pix.w = color1_pix.w;

  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixOverlayOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixOverlayOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixOverlayOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mixSaturationOp(CCL_WRITE(dst),
                           CCL_READ(value),
                           CCL_READ(color1),
                           CCL_READ(color2),
                           BOOL alpha_multiply,
                           BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }

  float4 color1_hsv = rgb_to_hsv(color1_pix);
  if (color1_pix.y != 0.0f) {
    float4 color2_hsv = rgb_to_hsv(color2_pix);
    color1_hsv.y = (1.0f - value_pix.x) * color1_hsv.y + value_pix.x * color2_hsv.y;
    color2_pix = hsv_to_rgb(color1_hsv);
  }
  else {
    color2_pix = color1_pix;
  }

  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixSaturationOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixSaturationOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixSaturationOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mixScreenOp(CCL_WRITE(dst),
                       CCL_READ(value),
                       CCL_READ(color1),
                       CCL_READ(color2),
                       BOOL alpha_multiply,
                       BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }

  color2_pix = 1.0f -
               ((1.0f - value_pix.x) + value_pix.x * (1.0f - color2_pix)) * (1.0f - color1_pix);
  color2_pix.w = color1_pix.w;

  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixScreenOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixScreenOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixScreenOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mixSoftLightOp(CCL_WRITE(dst),
                          CCL_READ(value),
                          CCL_READ(color1),
                          CCL_READ(color2),
                          BOOL alpha_multiply,
                          BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }

  /* first calculate non-fac based Screen mix */
  float4 color1m = 1.0f - color1_pix;
  float4 screen = 1.0f - (1.0f - color2_pix) * color1m;
  color2_pix = (1.0f - value_pix.x) * color1_pix +
               value_pix.x * (color1m * color2_pix * color1_pix + color1_pix * screen);
  color2_pix.w = color1_pix.w;

  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixSoftLightOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixSoftLightOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixSoftLightOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mixSubstractOp(CCL_WRITE(dst),
                          CCL_READ(value),
                          CCL_READ(color1),
                          CCL_READ(color2),
                          BOOL alpha_multiply,
                          BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }

  color2_pix = color1_pix - value_pix.x * color2_pix;
  color2_pix.w = color1_pix.w;

  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixSubtractOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixSubstractOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixSubstractOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mixValueOp(CCL_WRITE(dst),
                      CCL_READ(value),
                      CCL_READ(color1),
                      CCL_READ(color2),
                      BOOL alpha_multiply,
                      BOOL use_clamp)
{
  READ_DECL(value);
  READ_DECL(color1);
  READ_DECL(color2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  COPY_COORDS(color1, dst_coords);
  COPY_COORDS(color2, dst_coords);

  READ_IMG1(value, value_pix);
  READ_IMG4(color1, color1_pix);
  READ_IMG4(color2, color2_pix);

  if (alpha_multiply) {
    value_pix.x *= color2_pix.w;
  }

  float4 color1_hsv = rgb_to_hsv(color1_pix);
  float4 color2_hsv = rgb_to_hsv(color2_pix);
  color1_hsv.z = (1.0f - value_pix.x) * color1_hsv.z + value_pix.x * color2_hsv.z;
  color2_pix = hsv_to_rgb(color1_hsv);

  if (use_clamp) {
    color2_pix = clamp_to_normal_f4(color2_pix);
  }

  WRITE_IMG4(dst, color2_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MixValueOperation::execPixels(ExecutionManager &man)
{
  auto value = m_input_value->getPixels(this, man);
  auto color1 = m_input_color1->getPixels(this, man);
  auto color2 = m_input_color2->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL::mixValueOp, _1, value, color1, color2, m_valueAlphaMultiply, m_useClamp);
  computeWriteSeek(man, cpu_write, "mixValueOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addReadImgArgs(*color1);
    kernel->addReadImgArgs(*color2);
    kernel->addBoolArg(m_valueAlphaMultiply);
    kernel->addBoolArg(m_useClamp);
  });
}
