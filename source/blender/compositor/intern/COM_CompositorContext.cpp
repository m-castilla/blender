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

#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_main.h"
#include "BKE_scene.h"
#include "BLI_assert.h"
#include "DNA_userdef_types.h"
#include <stdio.h>

#include "COM_CompositorContext.h"
#include "COM_ComputeDevice.h"
#include "COM_GlobalManager.h"
#include "COM_defines.h"

const int MAX_IMG_DIM_SIZE = 16000;
CompositorContext::CompositorContext()
{
  m_exec_data = nullptr;
  m_quality = CompositorQuality::HIGH;
  m_cpu_work_threads = 0;
  m_previews = nullptr;
  m_inputs_scale = 0.2f;
  m_max_mem_cache_bytes = 0;
  m_max_disk_cache_bytes = 0;
  m_use_disk_cache = false;
  m_disk_cache_dir = "";
}

void CompositorContext::initialize()
{
}

void CompositorContext::deinitialize()
{
}

CompositorContext CompositorContext::build(const std::string &execution_id,
                                           CompositTreeExec *exec_data)
{

  CompositorContext context;
  context.m_exec_data = exec_data;

  auto rd = exec_data->rd;
  /* Make sure node tree has previews.
   * Don't create previews in advance, this is done when adding preview operations.
   * Reserved preview size is determined by render output for now.
   *
   * We fit the aspect into COM_PREVIEW_SIZE x COM_PREVIEW_SIZE image to avoid
   * insane preview resolution, which might even overflow preview dimensions.
   */
  const float aspect = rd->xsch > 0 ? (float)rd->ysch / (float)rd->xsch : 1.0f;
  int preview_size = context.getPreviewSize();
  int preview_width, preview_height;
  if (aspect < 1.0f) {
    preview_width = preview_size;
    preview_height = (int)(preview_size * aspect);
  }
  else {
    preview_width = (int)(preview_size / aspect);
    preview_height = preview_size;
  }
  BKE_node_preview_init_tree(exec_data->ntree, preview_width, preview_height, false);

  context.m_execution_id = execution_id;
  context.m_previews = exec_data->ntree->previews;

  /* initialize the CompositorContext */
  if (exec_data->rendering) {
    context.m_quality = static_cast<CompositorQuality>(exec_data->ntree->render_quality);
  }
  else {
    context.m_quality = static_cast<CompositorQuality>(exec_data->ntree->edit_quality);
  }

  context.m_max_mem_cache_bytes = (size_t)U.compositor_mem_cache_limit * 1024 * 1024;  // MB
  context.m_max_disk_cache_bytes = (size_t)U.compositor_disk_cache_limit * 1024 * 1024 *
                                   1024;  // GB
  context.m_disk_cache_dir = U.compositor_disk_cache_dir[0] == '\0' ? U.tempdir :
                                                                      U.compositor_disk_cache_dir;
  context.m_use_disk_cache = U.compositor_flag &
                             eUserpref_Compositor_Flag::USER_COMPOSITOR_DISK_CACHE_ENABLE;

  return context;
}

int CompositorContext::getMaxImgW() const
{
  if (GlobalMan->ComputeMan->canCompute()) {
    return GlobalMan->ComputeMan->getSelectedDevice()->getMaxImgW();
  }
  else {
    return MAX_IMG_DIM_SIZE;
  }
}
int CompositorContext::getMaxImgH() const
{
  if (GlobalMan->ComputeMan->canCompute()) {
    return GlobalMan->ComputeMan->getSelectedDevice()->getMaxImgH();
  }
  else {
    return MAX_IMG_DIM_SIZE;
  }
}
bool CompositorContext::isBreaked() const
{
  return m_exec_data->ntree->test_break && m_exec_data->ntree->test_break(m_exec_data->ntree->tbh);
}

void CompositorContext::updateDraw() const
{
  if (m_exec_data->ntree->update_draw) {
    m_exec_data->ntree->update_draw(this->m_exec_data->ntree->udh);
  }
}

int CompositorContext::getPreviewSize() const
{
  switch (m_exec_data->ntree->preview_size) {
    case NTREE_PREVIEW_SIZE_SMALL:
      return 150;
    case NTREE_PREVIEW_SIZE_MEDIUM:
      return 300;
    case NTREE_PREVIEW_SIZE_BIG:
      return 450;
    default:
      BLI_assert(!"Non implemented preview size");
      return 0;
  }
}

int CompositorContext::getCurrentFrame() const
{
  if (m_exec_data->rd) {
    return m_exec_data->rd->cfra;
  }
  else {
    BLI_assert(!"Unavailable frame number");
    return -1; /* this should never happen */
  }
}
