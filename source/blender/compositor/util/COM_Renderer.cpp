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

#include "BKE_global.h"
#include "BKE_node_offscreen.h"
#include "BKE_scene.h"
#include "BLI_assert.h"
#include "DNA_layer_types.h"
#include "DNA_scene_types.h"
#include "DRW_engine.h"
#include "GPU_framebuffer.h"
#include "IMB_imbuf.h"
#include "IMB_imbuf_types.h"
#include "RE_pipeline.h"

#include "COM_BufferUtil.h"
#include "COM_CompositorContext.h"
#include "COM_PixelsUtil.h"
#include "COM_Renderer.h"

static void freeGlRender(CompositGlRender *render)
{
  if (render) {
    if (render->result) {
      IMB_freeImBuf(render->result);
    }
    MEM_freeN(render);
  }
}

Renderer::Renderer()
{
  m_ctx = nullptr;
}

Renderer::~Renderer()
{
  deinitialize();
  for (auto render : m_cam_gl_renders.values()) {
    freeGlRender(render);
  }
}

void Renderer::initialize(CompositorContext *ctx)
{
  m_ctx = ctx;
}

void Renderer::deinitialize()
{
  m_sc_renders.clear();

  if (!m_ctx->isBreaked()) {
    /* Free non used cameras gl renders on this execution (because that means nodes where
     * deleted)*/
    blender::Vector<std::string> deleted_renders;
    for (auto item : m_cam_gl_renders.items()) {
      if (!m_used_cam_nodes.contains(item.key)) {
        freeGlRender(item.value);
        deleted_renders.append(item.key);
      }
    }
    for (auto name : deleted_renders) {
      m_cam_gl_renders.remove(name);
    }
  }
  m_used_cam_nodes.clear();
}

Render *Renderer::getSceneRender(Scene *selected_scene, ViewLayer *layer)
{
  bool is_current_scene = false;
  auto scene = m_ctx->getScene();
  if (scene && selected_scene) {
    auto selected_scene_uuid = selected_scene->id.orig_id ?
                                   selected_scene->id.orig_id->session_uuid :
                                   selected_scene->id.session_uuid;
    if (scene->id.session_uuid == selected_scene_uuid) {
      is_current_scene = true;
    }
  }

  if (is_current_scene && !m_ctx->isRendering() && layer && m_ctx->getbNodeTree()->auto_comp) {
    auto layer_name = std::string(layer->name);
    auto scene_uuid = scene->id.session_uuid;
    if (m_sc_renders.contains(scene_uuid) &&
        m_sc_renders.lookup(scene_uuid).contains(layer_name)) {
      return m_sc_renders.lookup(scene_uuid).lookup(layer_name);
    }
    else {
      /* save original state */
      bool orig_is_rendering = G.is_rendering;
      short orig_r_mode = scene->r.mode;
      short orig_r_scemode = scene->r.scemode;

      /* assure compositor and sequencer post-processing are disabled for this render */
      scene->r.scemode &= ~R_DOSEQ;
      scene->r.scemode &= ~R_DOCOMP;

      // scene->r.scemode &= R_BG_RENDER;
      scene->r.mode &= R_PERSISTENT_DATA;

      Render *re = RE_GetSceneRender(scene);
      if (re == NULL) {
        re = RE_NewSceneRender(scene);
      }

      RE_RenderFrame(re, m_ctx->getMain(), scene, layer, NULL, scene->r.cfra, false);

      if (!m_sc_renders.contains(scene_uuid)) {
        m_sc_renders.add(scene_uuid, {});
      }
      m_sc_renders.lookup(scene_uuid).add(layer_name, re);

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

/* Node name is unique */
static std::string getNodeName(bNode *camera_node)
{
  return std::string(camera_node->name);
}

bool Renderer::hasCameraNodeGlRender(bNode *camera_node)
{
  return m_cam_gl_renders.contains(getNodeName(camera_node));
}

CompositGlRender *Renderer::getCameraNodeGlRender(bNode *camera_node)
{
  auto name = getNodeName(camera_node);
  m_used_cam_nodes.add(name);
  return m_cam_gl_renders.lookup(getNodeName(camera_node));
}

void Renderer::setCameraNodeGlRender(bNode *camera_node, CompositGlRender *render)
{
  std::string name = getNodeName(camera_node);
  if (m_cam_gl_renders.contains(name)) {
    auto prev_render = m_cam_gl_renders.lookup(name);
    freeGlRender(prev_render);
    m_cam_gl_renders.remove(name);
  }
  m_cam_gl_renders.add_new(name, render);
}
