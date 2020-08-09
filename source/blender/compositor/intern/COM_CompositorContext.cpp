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
  m_inputs_scale = 1.0f;
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
  return std::move(context);
}

bool CompositorContext::isBreaked() const
{
  return m_bnodetree->test_break && m_bnodetree->test_break(m_bnodetree->tbh);
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
