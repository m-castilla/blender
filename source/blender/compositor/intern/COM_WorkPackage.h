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
 * Copyright 2011, Blender Foundation.
 */

#pragma once

#include "COM_Enums.h"

#include "BLI_rect.h"

#include <functional>
#include <ostream>

namespace blender::compositor {
// Forward Declarations.
class ExecutionGroup;

/**
 * \brief contains data about work that can be scheduled
 * \see WorkScheduler
 */
struct WorkPackage {
  eWorkPackageState state = eWorkPackageState::NotScheduled;
  std::atomic<bool> finished = false;
  std::function<void()> work_func;

  void execute()
  {
    work_func();
    finished = true;
  }

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:WorkPackage")
#endif
};

std::ostream &operator<<(std::ostream &os, const WorkPackage &work_package);

}  // namespace blender::compositor
