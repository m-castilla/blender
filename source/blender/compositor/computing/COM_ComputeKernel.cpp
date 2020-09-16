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
 * Copyright 2020, Blender Foundation.
 */

#include "COM_ComputeKernel.h"
#include "PIL_time.h"

ComputeKernel::ComputeKernel(std::string kernel_name) : m_kernel_name(kernel_name)
{
}

ComputeKernel::~ComputeKernel()
{
}

void ComputeKernel::addRandomSeedArg()
{
  addUInt64Arg(getRandomSeedArg(this));
}

// Seed initialization taken from BLI_rand.hh
uint64_t ComputeKernel::getRandomSeedArg(void *caller_object)
{
  uint64_t rng_seed = (uint64_t)(PIL_check_seconds_timer_i() & _UI64_MAX);
  rng_seed ^= (uint64_t)(uintptr_t)(caller_object);
  constexpr uint64_t lowseed = 0x330E;
  return (static_cast<uint64_t>(rng_seed) << 16) | lowseed;
}