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

#include "COM_Renderer.h"
#include "BKE_global.h"
#include "BLI_assert.h"
#include "COM_CompositorContext.h"
#include "DNA_layer_types.h"
#include "DNA_scene_types.h"
#include "RE_pipeline.h"

Renderer::Renderer()
{
}

Render *Renderer::getRender(CompositorContext *ctx, Scene *selected_scene, ViewLayer *layer)
{
  auto node_tree = ctx->getbNodeTree();

  bool is_current_scene = false;
  auto scene = ctx->getScene();
  if (scene && selected_scene) {
    auto selected_scene_uuid = selected_scene->id.orig_id ?
                                   selected_scene->id.orig_id->session_uuid :
                                   selected_scene->id.session_uuid;
    if (scene->id.session_uuid == selected_scene_uuid) {
      is_current_scene = true;
    }
  }

  if (is_current_scene && !ctx->isRendering() && layer && node_tree->auto_comp) {
    auto layer_name = std::string(layer->name);
    auto scene_uuid = scene->id.session_uuid;
    if (m_renders.contains(scene_uuid) && m_renders.lookup(scene_uuid).contains(layer_name)) {
      return m_renders.lookup(scene_uuid).lookup(layer_name);
    }
    else {
      /* save original state */
      bool orig_is_rendering = G.is_rendering;
      short orig_r_mode = scene->r.mode;
      short orig_r_scemode = scene->r.scemode;

      /* assure compositor and sequencer post-processing are disabled for this render */
      scene->r.scemode &= ~R_DOSEQ;
      scene->r.scemode &= ~R_DOCOMP;

      Render *re = RE_GetSceneRender(scene);
      if (re == NULL) {
        re = RE_NewSceneRender(scene);
      }

      RE_RenderFrame(re, ctx->getMain(), scene, layer, NULL, scene->r.cfra, false);

      if (!m_renders.contains(scene_uuid)) {
        m_renders.add(scene_uuid, {});
      }
      m_renders.lookup(scene_uuid).add(layer_name, re);

      /* restore previous state */
      G.is_rendering = orig_is_rendering;
      scene->r.scemode = orig_r_scemode;
      scene->r.mode = orig_r_mode;

      return re;
    }
  }
  else {
    return (selected_scene) ? RE_GetSceneRender(selected_scene) : NULL;
  }
}
