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

#pragma once

#include "COM_ComputeDevice.h"
#include "clew.h"

namespace blender::compositor {
class OpenCLManager;
class OpenCLPlatform;
class OpenCLKernel;
class OpenCLDevice : public ComputeDevice {
 private:
  cl_command_queue m_queue;
  OpenCLManager &m_man;
  OpenCLPlatform &m_platform;
  cl_device_id m_device_id;
  ComputeDeviceType m_device_type;
  cl_int m_vendor_id;
  size_t m_max_group_dims[3];
  int m_compute_units;
  int m_clock_freq;
  int m_global_mem_size;
  size_t m_max_img_w;
  size_t m_max_img_h;
  cl_image_format m_supported_formats;

 public:
  OpenCLDevice(OpenCLManager &man, OpenCLPlatform &platform, cl_device_id device_id);
  ~OpenCLDevice();

  void initialize() override;
  void enqueueWork(int work_width,
                   int work_height,
                   std::string kernel_name,
                   std::function<void(ComputeKernel *)> add_kernel_args_func) override;
  void waitQueueToFinish() override;

  void *allocGenericBuffer(ComputeAccess mem_access,
                           size_t bytes,
                           bool alloc_for_host_map) override;
  void *allocImageBuffer(ComputeAccess mem_access,
                         int width,
                         int height,
                         bool alloc_for_host_map) override;
  float *mapImageBufferToHostEnqueue(void *device_buffer,
                                     ComputeAccess mem_access,
                                     int width,
                                     int height,
                                     size_t &r_map_row_bytes);
  void unmapBufferFromHostEnqueue(void *device_buffer, void *host_mapped_buffer) override;
  void freeBuffer(void *device_buffer) override;

  int getNComputeUnits() override
  {
    return m_compute_units;
  }
  ComputeDeviceType getDeviceType() override
  {
    return m_device_type;
  }
  cl_device_id getDeviceId()
  {
    return m_device_id;
  }
  int getMaxImageWidth() override
  {
    return m_max_img_w;
  }
  int getMaxImageHeight() override
  {
    return m_max_img_h;
  }

  cl_command_queue getQueue()
  {
    return m_queue;
  }

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:OpenCLDevice")
#endif
};

}  // namespace blender::compositor
