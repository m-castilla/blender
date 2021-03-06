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

#pragma once

#include "COM_Node.h"
#include "COM_RenderLayersProg.h"
#include "DNA_node_types.h"

struct Render;
struct Scene;
struct ViewLayer;
/**
 * \brief RenderLayersNode
 * \ingroup Node
 */
class RenderLayersNode : public Node {
 public:
  RenderLayersNode(bNode *editorNode);
  void convertToOperations(NodeConverter &converter, CompositorContext &context) const;

 private:
  void testSocketLink(NodeConverter &converter,
                      CompositorContext &context,
                      NodeOutput *output,
                      RenderLayersProg *operation,
                      Scene *scene,
                      ViewLayer *view_layer,
                      bool is_preview) const;
  void testRenderLink(NodeConverter &converter,
                      CompositorContext &context,
                      Render *re,
                      ViewLayer *view_layer) const;

  void missingSocketLink(NodeConverter &converter, NodeOutput *output) const;
  void missingRenderLink(NodeConverter &converter) const;
};
