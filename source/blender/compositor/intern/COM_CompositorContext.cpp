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
#include "BKE_node.h"
#include "BKE_scene.h"
#include "COM_defines.h"
#include <stdio.h>

CompositorContext::CompositorContext()
{
  m_scene = nullptr;
  m_rd = nullptr;
  m_quality = CompositorQuality::HIGH;
  m_viewSettings = nullptr;
  m_displaySettings = nullptr;
  m_cpu_work_threads = 0;
  m_res_mode = DetermineResolutionMode::FromInput;
  m_previews = nullptr;
  m_inputs_scale = 0.2f;
}

CompositorContext CompositorContext::build(const std::string &execution_id,
                                           RenderData *rd,
                                           Scene *scene,
                                           bNodeTree *editingtree,
                                           bool rendering,
                                           const ColorManagedViewSettings *viewSettings,
                                           const ColorManagedDisplaySettings *displaySettings,
                                           const char *viewName)
{
  const int DEFAULT_BUFFER_CACHE_BYTES = 256 * 1024 * 1024;

  CompositorContext context;
  context.setbNodeTree(editingtree);

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

  context.setExecutionId(execution_id);
  context.setBufferCacheSize(DEFAULT_BUFFER_CACHE_BYTES);
  context.setViewName(viewName);
  context.setScene(scene);
  context.setPreviewHash(editingtree->previews);
  /* initialize the CompositorContext */
  if (rendering) {
    context.setQuality(static_cast<CompositorQuality>(editingtree->render_quality));
  }
  else {
    context.setQuality(static_cast<CompositorQuality>(editingtree->edit_quality));
  }
  context.setRendering(rendering);

  context.setRenderData(rd);
  context.setViewSettings(viewSettings);
  context.setDisplaySettings(displaySettings);

  return context;
}

bool CompositorContext::isBreaked() const
{
  return m_bnodetree->test_break && m_bnodetree->test_break(m_bnodetree->tbh);
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

int CompositorContext::getFramenumber() const
{
  if (this->m_rd) {
    return this->m_rd->cfra;
  }
  else {
    BLI_assert(!"Unavailable frame number");
    return -1; /* this should never happen */
  }
}
