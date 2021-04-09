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
#include "COM_ComputeManager.h"
#include "COM_OpenCLDevice.h"
#include "COM_OpenCLManager.h"
#include "COM_OpenCLPlatform.h"
#include "MEM_guardedalloc.h"

OpenCLKernel::OpenCLKernel(OpenCLManager &man,
                           OpenCLPlatform &platform,
                           OpenCLDevice *device,
                           const blender::StringRef kernel_name,
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
      m_args_count(0)
#ifdef DEBUG
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
#ifdef DEBUG
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
  if (m_device != new_device) {
    m_device = (OpenCLDevice *)new_device;
    m_initialized = false;
    initialize();
  }
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
  m_args_count = 0;
}

void OpenCLKernel::addArg(size_t arg_size, const void *arg)
{
  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, arg_size, arg));
  m_args_count++;
}

void OpenCLKernel::addBufferArg(void *cl_buffer)
{
  addArg(sizeof(cl_mem), &cl_buffer);
}

void OpenCLKernel::addSamplerArg(ComputeInterpolation interp, ComputeExtend extend)
{
  cl_sampler sampler = (cl_sampler)m_platform.getSampler(interp, extend);
  addArg(sizeof(cl_sampler), &sampler);
}

void OpenCLKernel::addIntArg(int value)
{
  addPrimitiveArg<cl_int, int>(value);
}
void OpenCLKernel::addBoolArg(bool value)
{
  addIntArg(value);
}

void OpenCLKernel::addInt3Arg(const int *value)
{
  addPrimitiveNArg<cl_int3, int, 3>(value);
}

void OpenCLKernel::addFloatArg(float value)
{
  addPrimitiveArg<cl_float, float>(value);
}

void OpenCLKernel::addFloat2Arg(const float *value)
{
  addPrimitiveNArg<cl_float2, float, 2>(value);
}

void OpenCLKernel::addFloat3Arg(const float *value)
{
  addPrimitiveNArg<cl_float3, float, 3>(value);
}

void OpenCLKernel::addFloat4Arg(const float *value)
{
  addPrimitiveNArg<cl_float4, float, 4>(value);
}

void OpenCLKernel::addFloat3ArrayArg(const float *float3_array,
                                     int n_elems,
                                     ComputeAccess mem_access)
{
  addPrimitiveNArrayArg<cl_float3, float, 3>(float3_array, n_elems, mem_access);
}

void OpenCLKernel::addFloat4ArrayArg(const float *float4_array,
                                     int n_elems,
                                     ComputeAccess mem_access)
{
  addPrimitiveNArrayArg<cl_float4, float, 4>(float4_array, n_elems, mem_access);
}

void OpenCLKernel::addIntArrayArg(int *int_array, int n_elems, ComputeAccess mem_access)
{
  addBufferArg(int_array, sizeof(cl_int), n_elems, mem_access);
}

void OpenCLKernel::addFloatArrayArg(float *float_array, int n_elems, ComputeAccess mem_access)
{
  addBufferArg(float_array, sizeof(cl_float), n_elems, mem_access);
}

void OpenCLKernel::addBufferArg(void *data, int elem_size, int n_elems, ComputeAccess mem_access)
{
  cl_int error;

  // if there is no data, just fill buffer with 1 elem as size (some OpenCL implementations
  // fail when passing null to kernels)
  size_t data_size = data ? elem_size * n_elems : elem_size;
  int mem_access_flag = m_platform.getMemoryAccessFlag(mem_access);
  cl_mem buffer = clCreateBuffer(
      m_platform.getContext(), mem_access_flag, data_size, NULL, &error);
  m_man.printIfError(error);
  m_args_buffers.append(buffer);

  if (data && mem_access != ComputeAccess::WRITE) {
    m_man.printIfError(clEnqueueWriteBuffer(
        m_device->getQueue(), buffer, CL_FALSE, 0, data_size, data, 0, NULL, NULL));
    m_work_enqueued = true;
  }

  addArg(sizeof(cl_mem), &buffer);
}