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

#include "COM_ExtendNode.h"
#include "COM_ExtendOperation.h"
#include "COM_UiConvert.h"

ExtendNode::ExtendNode(bNode *editorNode) : Node(editorNode)
{
  /* pass */
}

void ExtendNode::convertToOperations(NodeConverter &converter,
                                     const CompositorContext & /*context*/) const
{
  bNode *node = getbNode();
  NodeExtend *data = (NodeExtend *)node->storage;
  ExtendOperation *op = new ExtendOperation();
  op->setExtendMode(UiConvert::extendMode(data->extend_mode));
  op->setAddExtendH(data->add_extend_y);
  op->setAddExtendW(data->add_extend_x);
  op->setScale(data->scale);
  converter.addOperation(op);

  converter.mapInputSocket(getInputSocket(0), op->getInputSocket(0));
  converter.mapOutputSocket(getOutputSocket(), op->getOutputSocket());
}
