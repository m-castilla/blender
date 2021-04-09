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

#include "COM_ComputeManager.h"
#include "COM_ComputeDevice.h"

ComputeManager::ComputeManager() : m_device(nullptr), m_is_initialized(false)
{
}

ComputeManager::~ComputeManager()
{
}

void ComputeManager::initialize()
{
  if (!m_is_initialized) {
    auto devices = getDevices();
    auto selected_compute_units = 0;
    for (auto dev : devices) {
      dev->initialize();
      if (dev->getDeviceType() == ComputeDeviceType::GPU) {
        if (selected_compute_units < dev->getNComputeUnits()) {
          selected_compute_units = dev->getNComputeUnits();
          m_device = dev;
        }
      }
    }

    m_is_initialized = true;
  }
}
