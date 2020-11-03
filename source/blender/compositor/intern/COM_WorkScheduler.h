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

#include "COM_ExecutionGroup.h"

#include "BLI_threads.h"

#include "COM_WorkPackage.h"
#include "COM_defines.h"

/** \brief the workscheduler
 * \ingroup execution
 */
class WorkScheduler {

#if COM_CURRENT_THREADING_MODEL == COM_TM_QUEUE

  /**
   * \brief main thread loop for cpudevices
   * inside this loop new work is queried and being executed
   */
  static void *thread_execute_cpu(void *data);

#endif
 public:
  static void schedule(WorkPackage *package);

  /**
   * \brief initialize the WorkScheduler
   *
   */
  static void initialize(const CompositorContext &ctx);

  /**
   * \brief deinitialize the WorkScheduler
   * free all allocated resources
   */
  static void deinitialize();

  /**
   * \brief Start the execution
   * this methods will start the WorkScheduler. Inside this method all threads are initialized.
   * for every device a thread is created.
   * \see initialize Initialization and query of the number of devices
   */
  static void start(CompositorContext &context);

  /**
   * \brief stop the execution
   * All created thread by the start method are destroyed.
   * \see start
   */
  static void stop();

  /**
   * \brief wait for all work to be completed.
   */
  static void finish();

  static int current_thread_id();

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:WorkScheduler")
#endif
};
