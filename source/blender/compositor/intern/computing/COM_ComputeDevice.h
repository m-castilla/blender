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
#include "COM_compute_types.h"
#include <functional>
#include <string>
#include <unordered_map>

class ComputeKernel;
class ComputeDevice {
 private:
  bool m_initialized;

 public:
  virtual void initialize()
  {
    m_initialized = true;
  }
  bool isInitialized()
  {
    return m_initialized;
  }

  virtual void enqueueWork(int work_width,
                           int work_height,
                           std::string kernel_name,
                           std::function<void(ComputeKernel *)> add_kernel_args_func) = 0;
  virtual void waitQueueToFinish() = 0;

  virtual void *allocGenericBuffer(ComputeAccess mem_access,
                                   size_t bytes,
                                   bool alloc_for_host_map) = 0;
  virtual void *allocImageBuffer(ComputeAccess mem_access,
                                 int width,
                                 int height,
                                 bool alloc_for_host_map) = 0;
  virtual float *mapImageBufferToHostEnqueue(void *device_buffer,
                                             ComputeAccess mem_access,
                                             int width,
                                             int height,
                                             size_t &r_map_row_bytes) = 0;
  virtual void unmapBufferFromHostEnqueue(void *device_buffer, void *host_mapped_buffer) = 0;
  virtual void freeBuffer(void *device_buffer) = 0;

  virtual ComputeDeviceType getDeviceType() = 0;
  virtual int getNComputeUnits() = 0;
  virtual int getMaxImageWidth() = 0;
  virtual int getMaxImageHeight() = 0;

 protected:
  ComputeDevice();
  virtual ~ComputeDevice();

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:ComputeDevice")
#endif
};
