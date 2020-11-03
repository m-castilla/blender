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

#include "COM_RandomizeNode.h"
#include "COM_RandomizeOperation.h"
#include "COM_UiConvert.h"

RandomizeNode::RandomizeNode(bNode *editorNode) : Node(editorNode)
{
  /* pass */
}

void RandomizeNode::convertToOperations(NodeConverter &converter,
                                        CompositorContext & /*context*/) const
{
  bNode *node = getbNode();
  NodeRandomize *data = (NodeRandomize *)node->storage;
  RandomizeOperation *op = new RandomizeOperation();
  if (data->flag & CMP_NODEFLAG_RANDOMIZE_SEED) {
    op->setFixedSeed(data->seed);
  }
  op->setMinValue(data->min);
  op->setMaxValue(data->max);
  op->setVarianceDown(data->variance_down);
  op->setVarianceUp(data->variance_up);
  op->setVarianceSteps(data->variance_steps);
  converter.addOperation(op);

  converter.mapInputSocket(getInputSocket(0), op->getInputSocket(0));
  converter.mapOutputSocket(getOutputSocket(), op->getOutputSocket());
}
