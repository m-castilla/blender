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

#include "COM_MathBaseOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu_nocompat.h"

using namespace std::placeholders;
MathBaseOperation::MathBaseOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::VALUE);
  this->m_input1 = NULL;
  this->m_input2 = NULL;
  this->m_input3 = NULL;
  this->m_useClamp = false;
}

void MathBaseOperation::initExecution()
{
  this->m_input1 = this->getInputOperation(0);
  this->m_input2 = this->getInputOperation(1);
  this->m_input3 = this->getInputOperation(2);
}

void MathBaseOperation::deinitExecution()
{
  this->m_input1 = NULL;
  this->m_input2 = NULL;
  this->m_input3 = NULL;
}

void MathBaseOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_useClamp);
}

// void MathBaseOperation::determineResolution(int resolution[2], int preferredResolution[2])
//{
//  NodeOperationInput *socket;
//  int tempPreferredResolution[2] = {0, 0};
//  int tempResolution[2];
//
//  socket = this->getInputSocket(0);
//  socket->determineResolution(tempResolution, tempPreferredResolution);
//  if ((tempResolution[0] != 0) && (tempResolution[1] != 0)) {
//    this->setMainInputSocketIndex(0);
//  }
//  else {
//    this->setMainInputSocketIndex(1);
//  }
//  NodeOperation::determineResolution(resolution, preferredResolution);
//}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathAddOp(CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), BOOL use_clamp)
{
  READ_DECL(input1);
  READ_DECL(input2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(input1_pix.x + input2_pix.x) :
                             input1_pix.x + input2_pix.x;
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathAddOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathAddOp, _1, input1, input2, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathAddOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathSubtractOp(CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), BOOL use_clamp)
{
  READ_DECL(input1);
  READ_DECL(input2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(input1_pix.x - input2_pix.x) :
                             input1_pix.x - input2_pix.x;
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathSubtractOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathSubtractOp, _1, input1, input2, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathSubtractOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathMultiplyOp(CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), BOOL use_clamp)
{
  READ_DECL(input1);
  READ_DECL(input2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(input1_pix.x * input2_pix.x) :
                             input1_pix.x * input2_pix.x;
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathMultiplyOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathMultiplyOp, _1, input1, input2, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathMultiplyOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathDivideOp(CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), BOOL use_clamp)
{
  READ_DECL(input1);
  READ_DECL(input2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(input1_pix.x / input2_pix.x) :
                             input1_pix.x / input2_pix.x;
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathDivideOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathDivideOp, _1, input1, input2, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathDivideOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathSineOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(sinf(input1_pix.x)) : sinf(input1_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathSineOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathSineOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathSineOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathCosineOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(cosf(input1_pix.x)) : cosf(input1_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathCosineOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathCosineOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathCosineOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathTangentOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(tanf(input1_pix.x)) : tanf(input1_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathTangentOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathTangentOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathTangentOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathHiperbolicSineOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(sinhf(input1_pix.x)) : sinhf(input1_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathHyperbolicSineOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathHiperbolicSineOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathHiperbolicSineOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathHiperbolicCosineOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(coshf(input1_pix.x)) : coshf(input1_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathHyperbolicCosineOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathHiperbolicCosineOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathHiperbolicCosineOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathHiperbolicTangentOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(tanhf(input1_pix.x)) : tanhf(input1_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathHyperbolicTangentOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathHiperbolicTangentOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathHiperbolicTangentOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathArcSineOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  if (input1_pix.x <= 1 && input1_pix.x >= -1) {
    input1_pix.x = use_clamp ? clamp_to_normal(asinf(input1_pix.x)) : asinf(input1_pix.x);
  }
  else {
    input1_pix.x = 0.0f;
  }
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathArcSineOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathArcSineOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathArcSineOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathArcCosineOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  if (input1_pix.x <= 1 && input1_pix.x >= -1) {
    input1_pix.x = use_clamp ? clamp_to_normal(acosf(input1_pix.x)) : acosf(input1_pix.x);
  }
  else {
    input1_pix.x = 0.0f;
  }
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathArcCosineOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathArcCosineOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathArcCosineOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathArcTangentOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(atan(input1_pix.x)) : atan(input1_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathArcTangentOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathArcTangentOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathArcTangentOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathPowerOp(CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), BOOL use_clamp)
{
  READ_DECL(input1);
  READ_DECL(input2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  if (input1_pix.x >= 0) {
    input1_pix.x = powf(input1_pix.x, input2_pix.x);
  }
  else {
    float y_mod_1 = fmodf(input2_pix.x, 1);
    /* if input value is not nearly an integer, fall back to zero, nicer than straight rounding */
    if (y_mod_1 > 0.999f || y_mod_1 < 0.001f) {
      input1_pix.x = powf(input1_pix.x, floorf(input2_pix.x + 0.5f));
    }
    else {
      input1_pix.x = 0.0f;
    }
  }
  if (use_clamp) {
    input1_pix.x = clamp_to_normal(input1_pix.x);
  }
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathPowerOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathPowerOp, _1, input1, input2, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathPowerOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathLogarithmOp(CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), BOOL use_clamp)
{
  READ_DECL(input1);
  READ_DECL(input2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  if (input1_pix.x > 0 && input2_pix.x > 0) {
    input1_pix.x = logf(input1_pix.x) / logf(input2_pix.x);
    if (use_clamp) {
      input1_pix.x = clamp_to_normal(input1_pix.x);
    }
  }
  else {
    input1_pix.x = 0.0f;
  }

  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathLogarithmOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathLogarithmOp, _1, input1, input2, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathLogarithmOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathMinimumOp(CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), BOOL use_clamp)
{
  READ_DECL(input1);
  READ_DECL(input2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(fminf(input1_pix.x, input2_pix.x)) :
                             fminf(input1_pix.x, input2_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathMinimumOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathMinimumOp, _1, input1, input2, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathMinimumOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathMaximumOp(CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), BOOL use_clamp)
{
  READ_DECL(input1);
  READ_DECL(input2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(fmaxf(input1_pix.x, input2_pix.x)) :
                             fmaxf(input1_pix.x, input2_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathMaximumOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathMaximumOp, _1, input1, input2, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathMaximumOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathRoundOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(roundf(input1_pix.x)) : roundf(input1_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathRoundOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathRoundOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathRoundOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathLessThanOp(CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2))
{
  READ_DECL(input1);
  READ_DECL(input2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  input1_pix.x = input1_pix.x < input2_pix.x ? 1.0f : 0.0f;
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathLessThanOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathLessThanOp, _1, input1, input2);
  computeWriteSeek(man, cpu_write, "mathLessThanOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathGreaterThanOp(CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2))
{
  READ_DECL(input1);
  READ_DECL(input2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  input1_pix.x = input1_pix.x > input2_pix.x ? 1.0f : 0.0f;
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathGreaterThanOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathGreaterThanOp, _1, input1, input2);
  computeWriteSeek(man, cpu_write, "mathGreaterThanOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathModuloOp(CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), BOOL use_clamp)
{
  READ_DECL(input1);
  READ_DECL(input2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  if (input1_pix.x == 0.0f) {
    input1_pix.x = 0.0f;
  }
  else {
    input1_pix.x = use_clamp ? clamp_to_normal(fmodf(input1_pix.x, input2_pix.x)) :
                               fmodf(input1_pix.x, input2_pix.x);
  }
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathModuloOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathModuloOp, _1, input1, input2, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathModuloOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathAbsoluteOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(fabsf(input1_pix.x)) : fabsf(input1_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathAbsoluteOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathAbsoluteOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathAbsoluteOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathRadiansOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(DEG2RADF(input1_pix.x)) : DEG2RADF(input1_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathRadiansOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathRadiansOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathRadiansOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathDegreesOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(RAD2DEGF(input1_pix.x)) : RAD2DEGF(input1_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathDegreesOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathDegreesOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathDegreesOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathArcTan2Op(CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), BOOL use_clamp)
{
  READ_DECL(input1);
  READ_DECL(input2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(atan2f(input1_pix.x, input2_pix.x)) :
                             atan2f(input1_pix.x, input2_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathArcTan2Operation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathArcTan2Op, _1, input1, input2, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathArcTan2Op", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathFloorOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(floorf(input1_pix.x)) : floorf(input1_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathFloorOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathFloorOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathFloorOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathCeilOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(floorf(input1_pix.x)) : floorf(input1_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathCeilOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathCeilOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathCeilOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathFractOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(input1_pix.x - floorf(input1_pix.x)) :
                             input1_pix.x - floorf(input1_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathFractOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathFractOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathFractOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathSqrtOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  if (input1_pix.x > 0.0f) {
    input1_pix.x = use_clamp ? clamp_to_normal(sqrtf(input1_pix.x)) : sqrtf(input1_pix.x);
  }
  else {
    input1_pix.x = 0.0f;
  }

  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathSqrtOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathSqrtOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathSqrtOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathInverseSqrtOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  if (input1_pix.x > 0.0f) {
    input1_pix.x = use_clamp ? clamp_to_normal(1.0f / sqrtf(input1_pix.x)) :
                               1.0f / sqrtf(input1_pix.x);
  }
  else {
    input1_pix.x = 0.0f;
  }
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathInverseSqrtOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathInverseSqrtOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathInverseSqrtOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathSignOp(CCL_WRITE(dst), CCL_READ(input1))
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = compatible_signf(input1_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathSignOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathSignOp, _1, input1);
  computeWriteSeek(man, cpu_write, "mathSignOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathExponentOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = use_clamp ? clamp_to_normal(expf(input1_pix.x)) : expf(input1_pix.x);
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathExponentOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathExponentOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathExponentOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathTruncOp(CCL_WRITE(dst), CCL_READ(input1), BOOL use_clamp)
{
  READ_DECL(input1);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  input1_pix.x = (input1_pix.x >= 0.0f) ? floorf(input1_pix.x) : ceilf(input1_pix.x);
  if (use_clamp) {
    input1_pix.x = clamp_to_normal(input1_pix.x);
  }
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathTruncOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathTruncOp, _1, input1, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathTruncOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathSnapOp(CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), BOOL use_clamp)
{
  READ_DECL(input1);
  READ_DECL(input2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  if (input1_pix.x == 0.0f || input2_pix.x == 0.0f) {
    input1_pix.x = 0.0f;
  }
  else {
    input1_pix.x = floorf(input1_pix.x / input2_pix.x) * input2_pix.x;
  }
  if (use_clamp) {
    input1_pix.x = clamp_to_normal(input1_pix.x);
  }
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathSnapOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathSnapOp, _1, input1, input2, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathSnapOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathWrapOp(
    CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), CCL_READ(input3), BOOL use_clamp)
{
  READ_DECL(input1);
  READ_DECL(input2);
  READ_DECL(input3);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  READ_IMG(input3, dst_coords, input3_pix);
  input1_pix.x = wrapf(input1_pix.x, input2_pix.x, input3_pix.x);
  if (use_clamp) {
    input1_pix.x = clamp_to_normal(input1_pix.x);
  }
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathWrapOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto input3 = m_input3->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathWrapOp, _1, input1, input2, input3, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathWrapOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addReadImgArgs(*input3);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathPingPongOp(CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), BOOL use_clamp)
{
  READ_DECL(input1);
  READ_DECL(input2);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  input1_pix.x = fabsf(fractf((input1_pix.x - input2_pix.x) / (input2_pix.x * 2.0f)) *
                           input2_pix.x * 2.0f -
                       input2_pix.x);
  if (use_clamp) {
    input1_pix.x = clamp_to_normal(input1_pix.x);
  }
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathPingpongOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathPingPongOp, _1, input1, input2, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathPingPongOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathCompareOp(CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), CCL_READ(input3))
{
  READ_DECL(input1);
  READ_DECL(input2);
  READ_DECL(input3);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  READ_IMG(input3, dst_coords, input3_pix);
  input1_pix.x = (fabsf(input1_pix.x - input2_pix.x) <= fmaxf(input3_pix.x, 1e-5f)) ? 1.0f : 0.0f;
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathCompareOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto input3 = m_input3->getPixels(this, man);
  auto cpu_write = std::bind(CCL_NAMESPACE::mathCompareOp, _1, input1, input2, input3);
  computeWriteSeek(man, cpu_write, "mathCompareOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addReadImgArgs(*input3);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathMultiplyAddOp(
    CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), CCL_READ(input3), BOOL use_clamp)
{
  READ_DECL(input1);
  READ_DECL(input2);
  READ_DECL(input3);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  READ_IMG(input3, dst_coords, input3_pix);
  input1_pix.x = input1_pix.x * input2_pix.x + input3_pix.x;
  if (use_clamp) {
    input1_pix.x = clamp_to_normal(input1_pix.x);
  }
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathMultiplyAddOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto input3 = m_input3->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL_NAMESPACE::mathMultiplyAddOp, _1, input1, input2, input3, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathMultiplyAddOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addReadImgArgs(*input3);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathSmoothMinOp(
    CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), CCL_READ(input3), BOOL use_clamp)
{
  READ_DECL(input1);
  READ_DECL(input2);
  READ_DECL(input3);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  READ_IMG(input3, dst_coords, input3_pix);
  input1_pix.x = smoothminf(input1_pix.x, input2_pix.x, input3_pix.x);
  if (use_clamp) {
    input1_pix.x = clamp_to_normal(input1_pix.x);
  }
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathSmoothMinOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto input3 = m_input3->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL_NAMESPACE::mathSmoothMinOp, _1, input1, input2, input3, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathSmoothMinOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addReadImgArgs(*input3);
    kernel->addBoolArg(m_useClamp);
  });
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel mathSmoothMaxOp(
    CCL_WRITE(dst), CCL_READ(input1), CCL_READ(input2), CCL_READ(input3), BOOL use_clamp)
{
  READ_DECL(input1);
  READ_DECL(input2);
  READ_DECL(input3);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COORDS_TO_OFFSET(dst_coords);

  READ_IMG(input1, dst_coords, input1_pix);
  READ_IMG(input2, dst_coords, input2_pix);
  READ_IMG(input3, dst_coords, input3_pix);
  input1_pix.x = -smoothminf(-input1_pix.x, -input2_pix.x, input3_pix.x);
  if (use_clamp) {
    input1_pix.x = clamp_to_normal(input1_pix.x);
  }
  WRITE_IMG(dst, dst_coords, input1_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void MathSmoothMaxOperation::execPixels(ExecutionManager &man)
{
  auto input1 = m_input1->getPixels(this, man);
  auto input2 = m_input2->getPixels(this, man);
  auto input3 = m_input3->getPixels(this, man);
  auto cpu_write = std::bind(
      CCL_NAMESPACE::mathSmoothMaxOp, _1, input1, input2, input3, m_useClamp);
  computeWriteSeek(man, cpu_write, "mathSmoothMaxOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input1);
    kernel->addReadImgArgs(*input2);
    kernel->addReadImgArgs(*input3);
    kernel->addBoolArg(m_useClamp);
  });
}
