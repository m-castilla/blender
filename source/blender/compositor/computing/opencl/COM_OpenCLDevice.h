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

#ifndef __COM_OPENCLDEVICE_H__
#define __COM_OPENCLDEVICE_H__

#include "COM_ComputeDevice.h"
#include "COM_RectUtil.h"
#include "clew.h"

class OpenCLManager;
class OpenCLPlatform;
class OpenCLKernel;
class OpenCLDevice : public ComputeDevice {
 public:
  typedef struct OneElemImg {
    cl_mem img;
    float *elem_data;
  } OneElemImg;

 private:
  // limit to detect bugs (there shouldn't be a case of a kernel requesting more than 16 imgs)
  static const int MAX_ONE_ELEM_IMGS = 16;
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
  int m_max_img_w;
  int m_max_img_h;
  cl_image_format m_supported_formats;
  OneElemImg m_one_elem_imgs[MAX_ONE_ELEM_IMGS];

 public:
  OpenCLDevice(OpenCLManager &man, OpenCLPlatform &platform, cl_device_id device_id);
  ~OpenCLDevice();

  void initialize() override;
  void queueJob(PixelsRect &dst,
                std::string kernel_name,
                std::function<void(ComputeKernel *)> add_kernel_args_func) override;
  void waitQueueToFinish() override;

  void *memDeviceAlloc(
      MemoryAccess mem_access, int width, int height, int elem_chs, bool alloc_for_host_map);
  float *memDeviceToHostMapEnqueue(void *device_buffer,
                                   MemoryAccess mem_access,
                                   int width,
                                   int height,
                                   int elem_chs,
                                   size_t &r_map_row_pitch);
  void memDeviceToHostUnmapEnqueue(void *device_buffer, float *host_mapped_buffer);
  void memDeviceToHostCopyEnqueue(float *r_host_buffer,
                                  void *device_buffer,
                                  size_t host_row_bytes,
                                  MemoryAccess mem_access,
                                  int width,
                                  int height,
                                  int elem_chs);
  void memDeviceFree(void *device_buffer);
  void memHostToDeviceCopyEnqueue(void *device_buffer,
                                  float *host_buffer,
                                  size_t host_row_bytes,
                                  MemoryAccess mem_access,
                                  int width,
                                  int height,
                                  int elem_chs);

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
  int getMaxImgW() override
  {
    return m_max_img_w;
  }
  int getMaxImgH() override
  {
    return m_max_img_h;
  }

  OneElemImg getOneElemImg(int idx);
  cl_command_queue getQueue()
  {
    return m_queue;
  }

 private:
  void enqueueSplit(cl_command_queue cl_queue,
                    RectUtil::RectSplit split,
                    size_t offset_x,
                    size_t offset_y,
                    OpenCLKernel *kernel);
  void enqueueWork(cl_command_queue cl_queue,
                   size_t offset_x,
                   size_t offset_y,
                   size_t row_length,
                   size_t col_length,
                   size_t rect_w,
                   size_t rect_h,
                   OpenCLKernel *kernel);

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:OpenCLDevice")
#endif
};

#endif
