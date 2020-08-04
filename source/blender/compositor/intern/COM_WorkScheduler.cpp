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

#include <list>
#include <stdio.h>

#include "COM_CPUDevice.h"
#include "COM_WorkScheduler.h"
#include "COM_compositor.h"
#include "clew.h"

#include "MEM_guardedalloc.h"

#include "BLI_threads.h"
#include "PIL_time.h"

#include "BKE_global.h"

/// \brief list of all CPUDevices. for every hardware thread an instance of CPUDevice is created
static std::vector<CPUDevice *> g_cpudevices;
static ThreadLocal(CPUDevice *) g_thread_device;

#if COM_CURRENT_THREADING_MODEL == COM_TM_QUEUE
/** \brief list of all thread for every CPUDevice in cpudevices a thread exists. */
static ListBase g_cputhreads;
static bool g_cpuInitialized = false;
/** \brief all scheduled work for the cpu */
static ThreadQueue *g_cpuqueue;
#endif

#if COM_CURRENT_THREADING_MODEL == COM_TM_QUEUE
void *WorkScheduler::thread_execute_cpu(void *data)
{
  CPUDevice *device = (CPUDevice *)data;
  WorkPackage *work;
  BLI_thread_local_set(g_thread_device, device);
  while ((work = (WorkPackage *)BLI_thread_queue_pop(g_cpuqueue))) {
    device->execute(*work);
  }

  return NULL;
}
#endif

void WorkScheduler::schedule(WorkPackage *package)
{

#if COM_CURRENT_THREADING_MODEL == COM_TM_NOTHREAD
  CPUDevice device(0, 1);
  device.execute(*package);
#elif COM_CURRENT_THREADING_MODEL == COM_TM_QUEUE
  BLI_thread_queue_push(g_cpuqueue, (void *)package);
#endif
}

void WorkScheduler::start(const CompositorContext &context)
{
#if COM_CURRENT_THREADING_MODEL == COM_TM_QUEUE
  unsigned int index;
  g_cpuqueue = BLI_thread_queue_init();
  BLI_threadpool_init(&g_cputhreads, thread_execute_cpu, g_cpudevices.size());
  for (index = 0; index < g_cpudevices.size(); index++) {
    CPUDevice *device = g_cpudevices[index];
    BLI_threadpool_insert(&g_cputhreads, device);
  }
#endif
}
void WorkScheduler::finish()
{
#if COM_CURRENT_THREADING_MODEL == COM_TM_QUEUE
  BLI_thread_queue_wait_finish(g_cpuqueue);
#endif
}
void WorkScheduler::stop()
{
#if COM_CURRENT_THREADING_MODEL == COM_TM_QUEUE
  BLI_thread_queue_nowait(g_cpuqueue);
  BLI_threadpool_end(&g_cputhreads);
  BLI_thread_queue_free(g_cpuqueue);
  g_cpuqueue = NULL;
#endif
}

void WorkScheduler::initialize(CompositorContext &ctx)
{
#if COM_CURRENT_THREADING_MODEL == COM_TM_QUEUE
  /* deinitialize if number of threads doesn't match */
  int n_threads = ctx.getNCpuWorkThreads();
  if (g_cpudevices.size() != n_threads) {
    CPUDevice *device;

    while (!g_cpudevices.empty()) {
      device = g_cpudevices.back();
      g_cpudevices.pop_back();
      delete device;
    }
    if (g_cpuInitialized) {
      BLI_thread_local_delete(g_thread_device);
    }
    g_cpuInitialized = false;
  }

  /* initialize CPU threads */
  if (!g_cpuInitialized) {
    for (int index = 0; index < n_threads; index++) {
      CPUDevice *device = new CPUDevice(index, n_threads);
      g_cpudevices.push_back(device);
    }
    BLI_thread_local_create(g_thread_device);
    g_cpuInitialized = true;
  }
#endif
}

void WorkScheduler::deinitialize()
{
#if COM_CURRENT_THREADING_MODEL == COM_TM_QUEUE
  /* deinitialize CPU threads */
  if (g_cpuInitialized) {
    CPUDevice *device;
    while (g_cpudevices.size() > 0) {
      device = g_cpudevices.back();
      g_cpudevices.pop_back();
      delete device;
    }
    BLI_thread_local_delete(g_thread_device);
    g_cpuInitialized = false;
  }
#endif
}

int WorkScheduler::current_thread_id()
{
  CPUDevice *device = (CPUDevice *)BLI_thread_local_get(g_thread_device);
  return device->thread_id();
}
