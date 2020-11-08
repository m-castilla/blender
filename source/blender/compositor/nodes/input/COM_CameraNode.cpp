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

#include <cstring>

#include "COM_CameraNode.h"
#include "COM_CameraOperation.h"
#include "COM_GlobalManager.h"
#include "COM_Node.h"
#include "DNA_node_types.h"

CameraNode::CameraNode(bNode *editorNode) : Node(editorNode)
{
  /* pass */
}

void CameraNode::convertToOperations(NodeConverter &converter, CompositorContext &context) const
{
  auto node = getbNode();
  CameraGlRender *gl_render = GlobalMan->renderer()->getCameraNodeGlRender(node);

  CameraOperation *op = new CameraOperation();
  NodeOutput *output = this->getOutputSocket(0);
  op->setCameraGlRender(gl_render);
  converter.addOperation(op);
  converter.mapOutputSocket(output, op->getOutputSocket());

  converter.addPreview(op->getOutputSocket());
}
