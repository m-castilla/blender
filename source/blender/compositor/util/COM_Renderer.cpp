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
#include "BKE_node.h"
#include "BKE_node_camera_view.h"
#include "BKE_scene.h"
#include "BLI_assert.h"
#include "BLI_threads.h"
#include "DNA_camera_types.h"
#include "DNA_layer_types.h"
#include "DNA_object_types.h"
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

static void freeGlRender(CameraGlRender *render)
{
  if (render) {
    if (render->result) {
      IMB_freeImBuf(render->result);
    }
    delete render;
  }
}

Renderer::Renderer()
{
  m_ctx = nullptr;
}

Renderer::~Renderer()
{
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
  m_ctx = nullptr;
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

/* may renturn null when rendering failed by any reason */
CameraGlRender *Renderer::getCameraNodeGlRender(bNode *camera_node)
{
  static int gl_render_counter = 0;

  auto node_name = getNodeName(camera_node);
  m_used_cam_nodes.add(node_name);

  CameraGlRender *prev_render = nullptr;
  if (m_cam_gl_renders.contains(node_name)) {
    prev_render = m_cam_gl_renders.lookup(node_name);
  }
  if (!prev_render && m_ctx->isRendering()) {
    /* Opengl rendering fails when called from rendering pipeline. By returning null an empty image
     * will be drawn. */
    return nullptr;
  }

  Camera *camera = (Camera *)camera_node->id;
  NodeCamera *node_data = (NodeCamera *)camera_node->storage;
  Scene *scene = m_ctx->getScene();

  int draw_mode = node_data->draw_mode;
  int frame_offset = node_data->frame_offset;

  // copy used camera name
  std::string camera_name = std::string(
      camera ? camera->id.name + 2 : (scene->camera ? scene->camera->id.name + 2 : ""));

  auto ntree = m_ctx->getbNodeTree();
  auto tree_exec = m_ctx->getTreeExecData();
  if (ntree->auto_comp || !prev_render || !prev_render->result ||
      prev_render->draw_mode != draw_mode || prev_render->frame_offset != frame_offset ||
      prev_render->camera_name != camera_name) {
    freeGlRender(prev_render);
    m_cam_gl_renders.remove(node_name);

    ImBuf *render_result = tree_exec->camera_draw_fn(m_ctx->getScene(),
                                                     m_ctx->getViewLayer(),
                                                     m_ctx->getViewName(),
                                                     camera,
                                                     frame_offset,
                                                     (eDrawType)draw_mode,
                                                     tree_exec->job_context);
    CameraGlRender *new_render = nullptr;
    if (render_result) {
      new_render = new CameraGlRender();
      new_render->camera_name = camera_name;
      new_render->draw_mode = draw_mode;
      new_render->frame_offset = frame_offset;
      new_render->result = render_result;
      new_render->ssid = gl_render_counter;
      gl_render_counter++;

      setCameraNodeGlRender(camera_node, new_render);
    }
    return new_render;
  }
  else {
    return prev_render;
  }
}

void Renderer::setCameraNodeGlRender(bNode *camera_node, CameraGlRender *render)
{
  std::string name = getNodeName(camera_node);
  if (m_cam_gl_renders.contains(name)) {
    auto prev_render = m_cam_gl_renders.lookup(name);
    freeGlRender(prev_render);
    m_cam_gl_renders.remove(name);
  }
  m_cam_gl_renders.add_new(name, render);
}
