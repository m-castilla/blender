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

#include "BLI_map.hh"
#include "BLI_vector.hh"
#include "COM_ComputeKernel.h"
#include <memory>
#include <string>

#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif

using ComputeKernelDeleter = std::function<void(ComputeKernel *)>;
using ComputeKernelUniquePtr = std::unique_ptr<ComputeKernel, ComputeKernelDeleter>;

class ComputeKernel;
class ComputeDevice;
class ComputePlatform {
 private:
  blender::Map<std::string, blender::Vector<ComputeKernel *>> m_kernels;
  blender::Map<std::pair<int, int>, void *> m_samplers;
  int m_kernels_count;

 public:
  ComputeKernelUniquePtr takeKernel(const blender::StringRef kernel_name, ComputeDevice *device);
  void *getSampler(ComputeInterpolation interp, ComputeExtend extend);

  virtual ~ComputePlatform();

 protected:
  ComputePlatform();
  virtual ComputeKernel *createKernel(const blender::StringRef kernel_name,
                                      ComputeDevice *device) = 0;
  virtual void *createSampler(ComputeInterpolation interp, ComputeExtend extend) = 0;
  virtual void freeSampler(void *sampler) = 0;

 private:
  void recycleKernel(ComputeKernel *kernel);
  void cleanKernelsCache();

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:ComputePlatform")
#endif
};
