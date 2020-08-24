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

#include "COM_OpenCLKernel.h"
#include "BLI_assert.h"
#include "BLI_rect.h"
#include "COM_Buffer.h"
#include "COM_BufferUtil.h"
#include "COM_ComputeManager.h"
#include "COM_OpenCLDevice.h"
#include "COM_OpenCLManager.h"
#include "COM_OpenCLPlatform.h"
#include "COM_PixelsUtil.h"
#include "COM_Rect.h"
#include "MEM_guardedalloc.h"

OpenCLKernel::OpenCLKernel(OpenCLManager &man,
                           OpenCLPlatform &platform,
                           OpenCLDevice *device,
                           std::string kernel_name,
                           cl_kernel cl_kernel)
    : ComputeKernel(kernel_name),
      m_work_enqueued(false),
      m_platform(platform),
      m_man(man),
      m_device(device),
      m_cl_kernel(cl_kernel),
      m_max_group_size(0),
      m_group_size_multiple(0),
      m_initialized(false),
      m_args_count(0),
      m_one_elem_imgs_count(0)
#ifndef NDEBUG
      ,
      m_local_mem_used(0),
      m_private_mem_used(0)
#endif
{
}

OpenCLKernel::~OpenCLKernel()
{
  clearArgs();
  m_man.printIfError(clReleaseKernel(m_cl_kernel));
}

void OpenCLKernel::initialize()
{
  if (!m_initialized) {
    m_work_enqueued = false;
    auto device_id = m_device->getDeviceId();
    m_man.printIfError(clGetKernelWorkGroupInfo(m_cl_kernel,
                                                device_id,
                                                CL_KERNEL_WORK_GROUP_SIZE,
                                                sizeof(size_t),
                                                &m_max_group_size,
                                                NULL));
    m_man.printIfError(clGetKernelWorkGroupInfo(m_cl_kernel,
                                                device_id,
                                                CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
                                                sizeof(size_t),
                                                &m_group_size_multiple,
                                                NULL));
#if DEBUG
    cl_ulong local_mem_bytes;
    m_man.printIfError(clGetKernelWorkGroupInfo(m_cl_kernel,
                                                device_id,
                                                CL_KERNEL_LOCAL_MEM_SIZE,
                                                sizeof(cl_ulong),
                                                &local_mem_bytes,
                                                NULL));
    m_local_mem_used = (size_t)local_mem_bytes;

    cl_ulong private_mem_bytes;
    m_man.printIfError(clGetKernelWorkGroupInfo(m_cl_kernel,
                                                device_id,
                                                CL_KERNEL_PRIVATE_MEM_SIZE,
                                                sizeof(cl_ulong),
                                                &private_mem_bytes,
                                                NULL));
    m_private_mem_used = (size_t)private_mem_bytes;
#endif

    m_initialized = true;
  }
}

void OpenCLKernel::reset(ComputeDevice *new_device)
{
  clearArgs();
  m_device = (OpenCLDevice *)new_device;
  m_args_count = 0;
  m_one_elem_imgs_count = 0;
  m_initialized = false;
  initialize();
}

void OpenCLKernel::clearArgs()
{
  m_work_enqueued = false;
  for (cl_mem buffer : m_args_buffers) {
    clReleaseMemObject(buffer);
  }
  m_args_buffers.clear();

  for (void *data : m_args_datas) {
    MEM_freeN(data);
  }
  m_args_datas.clear();
}

void OpenCLKernel::addReadImgArgs(PixelsRect &pixels)
{
  cl_mem cl_img;
  if (pixels.is_single_elem) {
    auto elem = m_device->getOneElemImg(m_one_elem_imgs_count);
    cl_img = elem.img;
    m_one_elem_imgs_count++;

    size_t origin[3] = {0, 0, 0};
    size_t size[3] = {1, 1, 1};

    // write 1 entire pixel (4 channels) even if single elem has less channels
    size_t row_bytes = BufferUtil::calcBufferRowBytes(1, 4);
    int elem_chs = pixels.getElemChs();
    for (int i = 0; i < elem_chs; i++) {
      elem.elem_data[i] = pixels.single_elem[i];
    }
    m_man.printIfError(clEnqueueWriteImage(m_device->getQueue(),
                                           cl_img,
                                           CL_FALSE,
                                           origin,
                                           size,
                                           row_bytes,
                                           0,
                                           elem.elem_data,
                                           0,
                                           NULL,
                                           NULL));

    m_work_enqueued = true;
  }
  else {
    cl_img = (cl_mem)pixels.tmp_buffer->device.buffer;
  }

  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, sizeof(cl_mem), &cl_img));
  m_args_count++;

  int is_not_single = !pixels.is_single_elem;
  addIntArg(is_not_single);
}

std::function<void(int, int)> OpenCLKernel::addWriteImgArgs(PixelsRect &pixels)
{
  cl_mem cl_img = (cl_mem)pixels.tmp_buffer->device.buffer;
  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, sizeof(cl_mem), &cl_img));
  m_args_count++;

  int first_offset_arg_idx = m_args_count;
  std::function<void(int, int)> set_offset_func = [&, first_offset_arg_idx](int offset_x,
                                                                            int offset_y) {
    cl_int cl_offset_x = (cl_int)offset_x;
    cl_int cl_offset_y = (cl_int)offset_y;
    m_man.printIfError(
        clSetKernelArg(m_cl_kernel, first_offset_arg_idx, sizeof(cl_int), &cl_offset_x));
    m_man.printIfError(
        clSetKernelArg(m_cl_kernel, first_offset_arg_idx + 1, sizeof(cl_int), &cl_offset_y));
  };

  m_args_count += 2;

  return set_offset_func;
}

void OpenCLKernel::addSamplerArg(PixelsSampler &pix_sampler)
{
  cl_sampler sampler = (cl_sampler)m_platform.getSampler(pix_sampler);
  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, sizeof(cl_sampler), &sampler));
  m_args_count++;
}

void OpenCLKernel::addIntArg(int value)
{
  cl_int cint = (cl_int)value;
  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, sizeof(cl_int), &cint));
  m_args_count++;
}
void OpenCLKernel::addBoolArg(bool value)
{
  addIntArg(value);
}

void OpenCLKernel::addInt3Arg(const CCL::int3 &value)
{
  cl_int3 cl_i3;
  cl_i3.x = value.x;
  cl_i3.y = value.y;
  cl_i3.z = value.z;
  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, sizeof(cl_int3), &cl_i3));
  m_args_count++;
}

void OpenCLKernel::addFloatArg(float value)
{
  cl_float cfloat = (cl_float)value;
  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, sizeof(cl_float), &cfloat));
  m_args_count++;
}

void OpenCLKernel::addFloat2Arg(const CCL::float2 &value)
{
  cl_float2 cl_f2;
  cl_f2.x = value.x;
  cl_f2.y = value.y;
  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, sizeof(cl_float2), &cl_f2));
  m_args_count++;
}

void OpenCLKernel::addFloat3Arg(const CCL::float3 &value)
{
  cl_float3 cl_f3;
  cl_f3.x = value.x;
  cl_f3.y = value.y;
  cl_f3.z = value.z;
  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, sizeof(cl_float3), &cl_f3));
  m_args_count++;
}

void OpenCLKernel::addFloat4Arg(const CCL::float4 &value)
{
  cl_float4 cl_f4;
  cl_f4.x = value.x;
  cl_f4.y = value.y;
  cl_f4.z = value.z;
  cl_f4.w = value.w;
  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, sizeof(cl_float4), &cl_f4));
  m_args_count++;
}

cl_mem OpenCLKernel::addReadOnlyBufferArg(void *data, int elem_size, int n_elems)
{
  cl_int error;

  // if there is no data, just fill buffer with 1 elem as size (some OpenCL implementations
  // fail when passing null to kernels)
  size_t data_size = data ? elem_size * n_elems : elem_size;
  cl_mem buffer = clCreateBuffer(
      m_platform.getContext(), CL_MEM_READ_ONLY, data_size, NULL, &error);
  m_man.printIfError(error);
  m_args_buffers.push_back(buffer);

  if (data) {
    m_man.printIfError(clEnqueueWriteBuffer(
        m_device->getQueue(), buffer, CL_FALSE, 0, data_size, data, 0, NULL, NULL));
    m_work_enqueued = true;
  }

  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, sizeof(cl_mem), &buffer));
  m_args_count++;

  return buffer;
}

void OpenCLKernel::addFloat3CArrayArg(const CCL::float3 *float3_array, int n_elems)
{
  cl_float3 *data = nullptr;
  size_t elem_size = sizeof(cl_float3);
  if (float3_array) {
    size_t data_size = elem_size * n_elems;
    data = (cl_float3 *)MEM_mallocN(data_size, __func__);
    for (int i = 0; i < n_elems; i++) {
      const CCL::float3 &f3 = float3_array[i];
      cl_float4 &cl_f3 = data[i];
      cl_f3.x = f3.x;
      cl_f3.y = f3.y;
      cl_f3.z = f3.z;
    }
    m_args_datas.push_back(data);
  }
  addReadOnlyBufferArg(data, elem_size, n_elems);
}

void OpenCLKernel::addFloat4CArrayArg(const CCL::float4 *float4_array, int n_elems)
{
  cl_float4 *data = nullptr;
  size_t elem_size = sizeof(cl_float4);
  if (float4_array) {
    size_t data_size = elem_size * n_elems;
    data = (cl_float4 *)MEM_mallocN(data_size, __func__);
    for (int i = 0; i < n_elems; i++) {
      const CCL::float4 &f4 = float4_array[i];
      cl_float4 &cl_f4 = data[i];
      cl_f4.x = f4.x;
      cl_f4.y = f4.y;
      cl_f4.z = f4.z;
      cl_f4.w = f4.w;
    }
    m_args_datas.push_back(data);
  }
  addReadOnlyBufferArg(data, elem_size, n_elems);
}

void OpenCLKernel::addIntCArrayArg(int *int_array, int n_elems)
{
  addReadOnlyBufferArg(int_array, sizeof(cl_int), n_elems);
}

void OpenCLKernel::addFloatCArrayArg(float *float_array, int n_elems)
{
  addReadOnlyBufferArg(float_array, sizeof(cl_float), n_elems);
}