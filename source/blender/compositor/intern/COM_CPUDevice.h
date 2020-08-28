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

#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif
class WorkPackage;
/**
 * \brief class representing a CPU device.
 * \note for every hardware thread in the system a CPUDevice instance
 * will exist in the workscheduler.
 */
class CPUDevice {
 public:
  CPUDevice(int thread_id, int n_threads);

  /**
   * \brief execute a WorkPackage
   * \param work: the WorkPackage to execute
   */
  void execute(WorkPackage &work);

  int thread_id()
  {
    return m_thread_id;
  }

 protected:
  int m_thread_id;
  int m_n_threads;

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:CPUDevice")
#endif
};
