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

#include "BLI_threads.h"

#include "BLT_translation.h"

#include "BKE_node.h"
#include "BKE_scene.h"

#include "COM_BufferManager.h"
#include "COM_ComputeNoneManager.h"
#include "COM_Debug.h"
#include "COM_ExecutionSystem.h"
#include "COM_GlobalManager.h"
#include "COM_WorkScheduler.h"
#include "COM_compositor.h"
#include "clew.h"
#include <boost/lexical_cast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>  // streaming operators etc.

static ThreadMutex s_compositorMutex;
static bool is_compositorMutex_init = false;
static boost::uuids::random_generator uuid_generator;

void COM_execute(struct Main *main,
                 struct Depsgraph *depsgraph,
                 RenderData *rd,
                 Scene *scene,
                 bNodeTree *editingtree,
                 int rendering,
                 const ColorManagedViewSettings *viewSettings,
                 const ColorManagedDisplaySettings *displaySettings,
                 const char *viewName)
{
  /* initialize mutex, TODO this mutex init is actually not thread safe and
   * should be done somewhere as part of blender startup, all the other
   * initializations can be done lazily */
  if (is_compositorMutex_init == false) {
    BLI_mutex_init(&s_compositorMutex);
    is_compositorMutex_init = true;
  }

  BLI_mutex_lock(&s_compositorMutex);

  if (editingtree->test_break && editingtree->test_break(editingtree->tbh)) {
    // during editing multiple calls to this method can be triggered.
    // make sure one the last one will be doing the work.
    BLI_mutex_unlock(&s_compositorMutex);
    return;
  }

  if (!GlobalMan) {
    GlobalMan.reset(new GlobalManager());
  }

  DebugInfo::start_benchmark();

  /* build context */
  const std::string execution_id = boost::lexical_cast<std::string>(uuid_generator());
  CompositorContext context = CompositorContext::build(execution_id,
                                                       main,
                                                       depsgraph,
                                                       rd,
                                                       scene,
                                                       editingtree,
                                                       rendering,
                                                       viewSettings,
                                                       displaySettings,
                                                       viewName);
  int m_cpu_work_threads = BLI_system_thread_count();
  context.setNCpuWorkThreads(m_cpu_work_threads);

  /* set progress bar to 0% and status to init compositing */
  editingtree->progress(editingtree->prh, 0.0);
  editingtree->stats_draw(editingtree->sdh, IFACE_("Compositing"));

  GlobalMan->initialize(context);

  ExecutionSystem *system = new ExecutionSystem(context);

  system->execute();

  GlobalMan->deinitialize(context);
  delete system;

  DebugInfo::end_benchmark();

  DebugInfo::clear();

  BLI_mutex_unlock(&s_compositorMutex);
}

void COM_deinitialize()
{
  delete GlobalMan.get();
  GlobalMan.release();
  if (is_compositorMutex_init) {
    BLI_mutex_lock(&s_compositorMutex);
    WorkScheduler::deinitialize();
    is_compositorMutex_init = false;
    BLI_mutex_unlock(&s_compositorMutex);
    BLI_mutex_end(&s_compositorMutex);
  }
}
