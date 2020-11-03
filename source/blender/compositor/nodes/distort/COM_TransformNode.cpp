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

#include "COM_TransformNode.h"
#include "COM_ExecutionSystem.h"
#include "COM_GlobalManager.h"
#include "COM_SetValueOperation.h"
#include "COM_TransformOperation.h"
#include "COM_UiConvert.h"

TransformNode::TransformNode(bNode *editorNode) : Node(editorNode)
{
  /* pass */
}

void TransformNode::convertToOperations(NodeConverter &converter,
                                        CompositorContext & /*context*/) const
{
  auto sampler = GlobalMan->getContext()->getDefaultSampler();
  sampler.filter = UiConvert::pixelInterpolation(this->getbNode()->custom1);

  NodeInput *imageInput = this->getInputSocket(0);
  NodeInput *xInput = this->getInputSocket(1);
  NodeInput *yInput = this->getInputSocket(2);
  NodeInput *angleInput = this->getInputSocket(3);
  NodeInput *scaleInput = this->getInputSocket(4);

  TransformOperation *transformOp = new TransformOperation();
  transformOp->setSampler(sampler);
  converter.addOperation(transformOp);

  converter.mapInputSocket(imageInput, transformOp->getInputSocket(0));
  converter.mapInputSocket(xInput, transformOp->getInputSocket(1));
  converter.mapInputSocket(yInput, transformOp->getInputSocket(2));
  converter.mapInputSocket(angleInput, transformOp->getInputSocket(3));
  converter.mapInputSocket(scaleInput, transformOp->getInputSocket(4));

  converter.mapOutputSocket(getOutputSocket(), transformOp->getOutputSocket());
}
