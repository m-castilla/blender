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

#ifndef __COM_OPENCLPLATFORM_H__
#define __COM_OPENCLPLATFORM_H__

#include "COM_ComputeDevice.h"
#include "COM_ComputePlatform.h"
#include "clew.h"

class OpenCLManager;
struct PixelsImg;
class OpenCLPlatform : public ComputePlatform {
 private:
  cl_context m_context;
  cl_program m_program;
  OpenCLManager &m_man;

 public:
  OpenCLPlatform(OpenCLManager &man, cl_context context, cl_program program);
  ~OpenCLPlatform();

  cl_context getContext()
  {
    return m_context;
  }
  cl_program getProgram()
  {
    return m_program;
  }

  const cl_image_format *getImageFormat(int elem_chs) const;
  int getMemoryAccessFlag(MemoryAccess mem_access) const;

 protected:
  ComputeKernel *createKernel(std::string kernel_name, ComputeDevice *device) override;
  void *createSampler(PixelsSampler pix_sampler) override;

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:OpenCLPlatform")
#endif
};

#endif
