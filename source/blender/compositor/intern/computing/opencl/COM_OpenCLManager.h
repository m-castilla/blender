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

#include "BLI_span.hh"
#include "BLI_vector.hh"
#include "COM_ComputeManager.h"
#include "clew.h"

namespace blender::compositor {
class OpenCLPlatform;
class OpenCLDevice;
class ComputeDevice;
class OpenCLManager : public ComputeManager {
 private:
  blender::Vector<OpenCLPlatform *> m_platforms;
  blender::Vector<OpenCLDevice *> m_devices;

 public:
  OpenCLManager();
  void initialize() override;
  void printIfError(int result_code);
  ~OpenCLManager();

 protected:
  blender::Span<ComputeDevice *> getDevices() override;
  ComputeType getComputeType() override
  {
    return ComputeType::OPENCL;
  }

 private:
  std::pair<cl_device_id *, int> checkDevicesRequiredCapabilities(cl_device_id *devices,
                                                                  int n_devices);

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:OpenCLManager")
#endif
};

}  // namespace blender::compositor
