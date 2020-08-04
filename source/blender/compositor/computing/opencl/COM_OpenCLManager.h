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

#ifndef __COM_OPENCLMANAGER_H__
#define __COM_OPENCLMANAGER_H__

#include "COM_ComputeManager.h"
#include "clew.h"

class OpenCLPlatform;
class OpenCLDevice;
class ComputeDevice;
class OpenCLManager : public ComputeManager {
 public:
  OpenCLManager();
  void initialize() override;
  void printIfError(int result_code);
  ~OpenCLManager();

 protected:
  std::vector<ComputeDevice *> getDevices() override;
  ComputeType getComputeType() override
  {
    return ComputeType::OPENCL;
  }

 private:
  std::pair<cl_device_id *, int> checkDevicesRequiredCapabilities(cl_device_id *devices,
                                                                  int n_devices);
  std::vector<OpenCLPlatform *> m_platforms;
  std::vector<OpenCLDevice *> m_devices;

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:OpenCLManager")
#endif
};

#endif
