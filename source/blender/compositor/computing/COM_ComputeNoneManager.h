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

#ifndef __COM_COMPUTENONEMANAGER_H__
#define __COM_COMPUTENONEMANAGER_H__

#include "COM_ComputeManager.h"
#include <vector>

class ComputeNoneManager : public ComputeManager {
 protected:
  std::vector<ComputeDevice *> getDevices() override
  {
    return std::vector<ComputeDevice *>();
  };
  ComputeType getComputeType() override
  {
    return ComputeType::NONE;
  }

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:ComputeNoneManager")
#endif
};

#endif
