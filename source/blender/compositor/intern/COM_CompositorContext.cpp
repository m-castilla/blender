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

#include "COM_CompositorContext.h"
#include "BKE_context.h"
#include "BKE_global.h"
#include "BKE_main.h"
#include "BKE_node.h"
#include "BKE_scene.h"
#include "BLI_assert.h"
#include "COM_defines.h"
#include "DNA_userdef_types.h"
#include <stdio.h>

CompositorContext::CompositorContext()
{
  m_scene = nullptr;
  m_rd = nullptr;
  m_quality = CompositorQuality::HIGH;
  m_viewSettings = nullptr;
  m_displaySettings = nullptr;
  m_cpu_work_threads = 0;
  m_previews = nullptr;
  m_inputs_scale = 0.2f;
  m_max_mem_cache_bytes = 0;
  m_max_disk_cache_bytes = 0;
  m_use_disk_cache = false;
  m_disk_cache_dir = "";
  m_bnodetree = nullptr;
  m_rendering = false;
  m_viewName = nullptr;
  m_main = nullptr;
  m_depsgraph = nullptr;
  m_view_layer = nullptr;
}

CompositorContext CompositorContext::build(const std::string &execution_id,
                                           struct Main *main,
                                           struct Depsgraph *depsgraph,
                                           RenderData *rd,
                                           Scene *scene,
                                           ViewLayer *view_layer,
                                           bNodeTree *editingtree,
                                           bool rendering,
                                           const ColorManagedViewSettings *viewSettings,
                                           const ColorManagedDisplaySettings *displaySettings,
                                           const char *viewName)
{
  CompositorContext context;
  context.m_main = main;
  context.m_depsgraph = depsgraph;
  context.m_bnodetree = editingtree;
  context.m_view_layer = view_layer;

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
  BKE_node_preview_init_tree(editingtree, preview_width, preview_height, false);

  context.m_execution_id = execution_id;
  context.m_viewName = viewName;
  context.m_scene = scene;
  context.m_previews = editingtree->previews;

  /* initialize the CompositorContext */
  if (rendering) {
    context.m_quality = static_cast<CompositorQuality>(editingtree->render_quality);
  }
  else {
    context.m_quality = static_cast<CompositorQuality>(editingtree->edit_quality);
  }
  context.m_rendering = rendering;
  context.m_rd = rd;
  context.m_viewSettings = viewSettings;
  context.m_displaySettings = displaySettings;

  context.m_max_mem_cache_bytes = (size_t)U.compositor_mem_cache_limit * 1024 * 1024;  // MB
  context.m_max_disk_cache_bytes = (size_t)U.compositor_disk_cache_limit * 1024 * 1024 *
                                   1024;  // GB
  context.m_disk_cache_dir = U.compositor_disk_cache_dir ? U.compositor_disk_cache_dir : "";
  context.m_use_disk_cache = U.compositor_flag &
                             eUserpref_Compositor_Flag::USER_COMPOSITOR_DISK_CACHE_ENABLE;

  return context;
}

bool CompositorContext::isBreaked() const
{
  return m_bnodetree->test_break && m_bnodetree->test_break(m_bnodetree->tbh);
}

void CompositorContext::updateDraw() const
{
  if (m_bnodetree->update_draw) {
    m_bnodetree->update_draw(this->m_bnodetree->udh);
  }
}

int CompositorContext::getPreviewSize() const
{
  switch (m_bnodetree->preview_size) {
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
  if (this->m_rd) {
    return this->m_rd->cfra;
  }
  else {
    BLI_assert(!"Unavailable frame number");
    return -1; /* this should never happen */
  }
}
