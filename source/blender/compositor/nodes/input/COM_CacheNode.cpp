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

#include "COM_CacheNode.h"
#include "COM_CacheOperation.h"
#include "COM_ExecutionSystem.h"

CacheNode::CacheNode(bNode *editorNode) : Node(editorNode)
{
  /* pass */
}

void CacheNode::convertToOperations(NodeConverter &converter,
                                    CompositorContext & /*context*/) const
{
  NodeInput *input = this->getInputSocket(0);
  bNode *editorNode = this->getbNode();
  bool is_persistent = editorNode->custom1;

  CacheOperation *cache_op = new CacheOperation();
  cache_op->setIsPersistent(is_persistent);
  converter.addOperation(cache_op);
  converter.mapInputSocket(input, cache_op->getInputSocket(0));
  converter.mapOutputSocket(this->getOutputSocket(0), cache_op->getOutputSocket());
}
