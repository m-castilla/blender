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

#include "COM_TranslateNode.h"

#include "COM_ExecutionSystem.h"
#include "COM_TranslateOperation.h"
TranslateNode::TranslateNode(bNode *editorNode) : Node(editorNode)
{
  /* pass */
}

void TranslateNode::convertToOperations(NodeConverter &converter,
                                        const CompositorContext &context) const
{
  bNode *bnode = this->getbNode();
  NodeTranslateData *data = (NodeTranslateData *)bnode->storage;

  NodeInput *inputSocket = this->getInputSocket(0);
  NodeInput *inputXSocket = this->getInputSocket(1);
  NodeInput *inputYSocket = this->getInputSocket(2);
  NodeOutput *outputSocket = this->getOutputSocket(0);

  TranslateOperation *op = new TranslateOperation();
  op->setWrapping(data->wrap_axis);
  op->setRelative(data->relative);

  converter.addOperation(op);
  converter.mapInputSocket(inputSocket, op->getInputSocket(0));
  converter.mapInputSocket(inputXSocket, op->getInputSocket(1));
  converter.mapInputSocket(inputYSocket, op->getInputSocket(2));
  converter.mapOutputSocket(outputSocket, op->getOutputSocket(0));
}
