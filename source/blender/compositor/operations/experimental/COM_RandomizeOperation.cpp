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

#include "COM_RandomizeOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_GlobalManager.h"
#include "COM_kernel_cpu.h"
#include <algorithm>

using namespace std::placeholders;
RandomizeOperation::RandomizeOperation()
    : NodeOperation(),
      m_min_value(0),
      m_max_value(0),
      m_variance_down(0),
      m_variance_up(0),
      m_use_fixed_seed(false),
      m_fixed_seed(0),
      m_variance_table(nullptr)
{
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::VALUE);
}

void RandomizeOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_use_fixed_seed);
  if (m_use_fixed_seed) {
    hashParam(m_fixed_seed);
  }
  hashParam(m_min_value);
  hashParam(m_max_value);
  hashParam(m_variance_down);
  hashParam(m_variance_up);
  hashParam(m_variance_steps);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel randomizeOp(CCL_WRITE(dst),
                       CCL_READ(value),
                       float min_value,
                       float max_value,
                       float variance_down,
                       float variance_up,
                       uint64_t random_seed)
{
  READ_DECL(value);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);
  uint64_t random_state = random_seed;

  COPY_COORDS(value, dst_coords);
  READ_IMG1(value, value_pix);
  float from_value = value_pix.x - variance_down;
  if (from_value < min_value) {
    from_value = min_value;
  }
  float to_value = value_pix.x + variance_up;
  if (to_value > max_value) {
    to_value = max_value;
  }
  float random_add = random_float(&random_state, dst_coords) * (to_value - from_value);
  value_pix.x = from_value + random_add;

  WRITE_IMG1(dst, value_pix);

  CPU_LOOP_END;
}

ccl_kernel randomizeStepsOp(CCL_WRITE(dst),
                            CCL_READ(value),
                            float min_value,
                            float max_value,
                            uint64_t random_seed,
                            ccl_constant float *steps,
                            int n_steps,
                            float step_incr)
{
  READ_DECL(value);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(value, dst_coords);
  READ_IMG1(value, value_pix);
  float val = value_pix.x;
  if (val < min_value) {
    val = min_value;
  }
  if (val > max_value) {
    val = max_value;
  }
  float rel_value = val - min_value;
  int from_step = (int)(rel_value / step_incr);
  if (from_step >= n_steps - 1) {
    from_step = n_steps - 2;
  }
  int to_step = from_step + 1;
  float factor = (rel_value - (step_incr * from_step)) / step_incr;
  float from_random = steps[from_step];
  float to_random = steps[to_step];
  if (to_random < from_random) {
    float tmp = from_random;
    from_random = to_random;
    to_random = tmp;
  }
  value_pix.x = from_random + (to_random - from_random) * factor;

  WRITE_IMG1(dst, value_pix);

  CPU_LOOP_END;
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void RandomizeOperation::execPixels(ExecutionManager &man)
{
  auto value = getInputOperation(0)->getPixels(this, man);
  auto seed = m_use_fixed_seed ? m_fixed_seed : ComputeKernel::getRandomSeedArg(this);
  if (m_variance_steps > 1) {
    float random_range = m_max_value - m_min_value;
    float *steps = new float[m_variance_steps];
    float step_value_incr = random_range / m_variance_steps;
    CCL::int2 coords = {0, 0};
    int i = 0;
    while (i < m_variance_steps) {
      float step_value = step_value_incr * i;
      float from_value = step_value - m_variance_down;
      if (from_value < m_min_value) {
        from_value = m_min_value;
      }
      float to_value = step_value + m_variance_up;
      if (to_value > m_max_value) {
        to_value = m_max_value;
      }
      float random_add = CCL::random_float(&seed, coords) * (to_value - from_value);
      steps[i] = from_value + random_add;
      i++;
      coords.x = i;
      coords.y = i;
    }
    auto cpu_write = std::bind(CCL::randomizeStepsOp,
                               _1,
                               value,
                               m_min_value,
                               m_max_value,
                               seed,
                               steps,
                               m_variance_steps,
                               step_value_incr);
    computeWriteSeek(man, cpu_write, "randomizeStepsOp", [&](ComputeKernel *kernel) {
      kernel->addReadImgArgs(*value);
      kernel->addFloatArg(m_min_value);
      kernel->addFloatArg(m_max_value);
      kernel->addUInt64Arg(seed);
      kernel->addFloatArrayArg(steps, m_variance_steps, MemoryAccess::READ);
      kernel->addIntArg(m_variance_steps);
      kernel->addFloatArg(step_value_incr);
    });
    delete[] steps;
  }
  else {
    auto cpu_write = std::bind(CCL::randomizeOp,
                               _1,
                               value,
                               m_min_value,
                               m_max_value,
                               m_variance_down,
                               m_variance_up,
                               seed);
    computeWriteSeek(man, cpu_write, "randomizeOp", [&](ComputeKernel *kernel) {
      kernel->addReadImgArgs(*value);
      kernel->addFloatArg(m_min_value);
      kernel->addFloatArg(m_max_value);
      kernel->addFloatArg(m_variance_down);
      kernel->addFloatArg(m_variance_up);
      kernel->addUInt64Arg(seed);
    });
  }
}