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

#include "BLI_span.hh"
#include "COM_compute_types.h"
#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif

namespace blender::compositor {

class ComputeKernel;
class ComputeDevice;
class ComputeManager {
 private:
  ComputeDevice *m_device;
  bool m_is_initialized;

 public:
  virtual void initialize();
  bool canCompute() const
  {
    return m_device != nullptr;
  }
  bool isInitialized()
  {
    return m_is_initialized;
  }
  ComputeDevice *getSelectedDevice()
  {
    return m_device;
  }

  virtual ComputeType getComputeType() = 0;

  virtual ~ComputeManager() = 0;

 protected:
  ComputeManager();

  virtual blender::Span<ComputeDevice *> getDevices() = 0;

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:ComputeManager")
#endif
};

}  // namespace blender::compositor
