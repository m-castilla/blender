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
 * Copyright 2021, Blender Foundation.
 */

#pragma once

#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif

#include "BLI_rect.h"
#include "BLI_string_ref.hh"
#include "COM_compute_types.h"

namespace blender::compositor {

class ComputeDevice;
struct GPUBuffer;
class ComputeKernel {
 private:
  std::string m_kernel_name;

 public:
  std::string getKernelName()
  {
    return m_kernel_name;
  }
  virtual void reset(ComputeDevice *new_device) = 0;
  virtual void clearArgs() = 0;
  virtual void addBufferArg(void *device_buffer) = 0;
  virtual void addBufferArg(const GPUBuffer *gpu_buffer) = 0;
  virtual void addSamplerArg(ComputeInterpolation interp, ComputeExtend extend) = 0;
  virtual void addBoolArg(bool value) = 0;
  virtual void addIntArg(int value) = 0;
  virtual void addInt3Arg(const int *value) = 0;
  virtual void addFloatArg(float value) = 0;
  virtual void addFloat2Arg(const float *value) = 0;
  virtual void addFloat3Arg(const float *value) = 0;
  virtual void addFloat4Arg(const float *value) = 0;

  virtual void addFloat3ArrayArg(const float *float3_array,
                                 int n_elems,
                                 ComputeAccess mem_access) = 0;
  virtual void addFloat4ArrayArg(const float *float4_array,
                                 int n_elems,
                                 ComputeAccess mem_access) = 0;
  virtual void addIntArrayArg(int *value, int n_elems, ComputeAccess mem_access) = 0;
  virtual void addFloatArrayArg(float *float_array, int n_elems, ComputeAccess mem_access) = 0;

  virtual bool hasWorkEnqueued() = 0;

  virtual ~ComputeKernel();

 protected:
  ComputeKernel(const blender::StringRef kernel_name);

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:ComputeKernel")
#endif
};

}  // namespace blender::compositor
