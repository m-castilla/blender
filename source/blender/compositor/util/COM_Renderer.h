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

#pragma once

#include "BLI_Map.hh"
#include "BLI_set.hh"
#include "DNA_object_enums.h"

struct Scene;
struct ViewLayer;
struct Render;
struct GPUOffScreen;
struct Main;
struct CompositGlRender;
class CompositorContext;
class Renderer {
 private:
  blender::Map<unsigned int, blender::Map<std::string, Render *>> m_sc_renders;
  blender::Map<std::string, CompositGlRender *> m_cam_gl_renders;
  blender::Set<std::string> m_used_cam_nodes;
  CompositorContext *m_ctx;

 public:
  Renderer();
  ~Renderer();
  void initialize(CompositorContext *ctx);
  void deinitialize();
  Render *getSceneRender(Scene *scene, ViewLayer *layer);
  bool hasCameraNodeGlRender(bNode *camera_node);
  CompositGlRender *getCameraNodeGlRender(bNode *camera_node);
  void setCameraNodeGlRender(bNode *camera_node, CompositGlRender *render);
};
