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
      m_man(man),
      m_platform(platform),
      m_cl_kernel(cl_kernel),
      m_device(device),
      m_max_group_size(0),
      m_group_size_multiple(0),
      m_initialized(false),
      m_args_count(0)
#ifndef NDEBUG
      ,
      m_local_mem_used(0),
      m_private_mem_used(0)
#endif
{
}

OpenCLKernel::~OpenCLKernel()
{
  m_man.printIfError(clReleaseKernel(m_cl_kernel));
}

void OpenCLKernel::initialize()
{
  if (!m_initialized) {
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
  m_device = (OpenCLDevice *)new_device;
  m_args_count = 0;
  m_initialized = false;
  initialize();
}

void OpenCLKernel::addReadImgArgs(PixelsRect &pixels)
{
  auto img = pixels.pixelsImg();
  cl_mem cl_img = pixels.is_single_elem ? m_device->getEmptyImg() :
                                          (cl_mem)pixels.tmp_buffer->device.buffer;
  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, sizeof(cl_mem), &cl_img));
  m_args_count++;

  addBoolArg(pixels.is_single_elem);

  // add single element argument
  float float4_elem[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  if (pixels.is_single_elem) {
    memcpy(&float4_elem, pixels.single_elem, pixels.single_elem_chs * sizeof(float));
  }
  addFloat4Arg(float4_elem);
}

cl_mem OpenCLKernel::addWriteImgArgs(PixelsRect &pixels)
{
  auto img = pixels.pixelsImg();
  cl_mem cl_img = (cl_mem)pixels.tmp_buffer->device.buffer;
  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, sizeof(cl_mem), &cl_img));
  m_args_count++;

  addIntArg(pixels.xmin);
  addIntArg(pixels.ymin);

  return cl_img;
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
void OpenCLKernel::addFloatArg(float value)
{
  cl_float cfloat = (cl_float)value;
  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, sizeof(cl_float), &cfloat));
  m_args_count++;
}

void OpenCLKernel::addFloat2Arg(float *value)
{
  cl_float2 *cfloat2 = (cl_float2 *)value;
  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, sizeof(cl_float2), cfloat2));
  m_args_count++;
}

void OpenCLKernel::addFloat3Arg(float *value)
{
  cl_float3 *cfloat3 = (cl_float3 *)value;
  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, sizeof(cl_float3), cfloat3));
  m_args_count++;
}

void OpenCLKernel::addFloat4Arg(float *value)
{
  cl_float4 *cfloat4 = (cl_float4 *)value;
  m_man.printIfError(clSetKernelArg(m_cl_kernel, m_args_count, sizeof(cl_float4), cfloat4));
  m_args_count++;
}
