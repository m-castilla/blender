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

#include "COM_SocketProxyNode.h"
#include "COM_ExecutionSystem.h"
#include "COM_SetColorOperation.h"
#include "COM_SetValueOperation.h"
#include "COM_SetVectorOperation.h"
#include "COM_SocketProxyOperation.h"

SocketProxyNode::SocketProxyNode(bNode *editorNode,
                                 bNodeSocket *editorInput,
                                 bNodeSocket *editorOutput,
                                 bool use_conversion)
    : Node(editorNode, false), m_use_conversion(use_conversion)
{
  this->addInputSocket(SocketType::DYNAMIC, editorInput);
  this->addOutputSocket(SocketType::DYNAMIC, editorOutput);
  // DataType dt;

  // dt = DataType::VALUE;
  // if (editorInput->type == SOCK_RGBA) {
  //  dt = DataType::COLOR;
  //}
  // if (editorInput->type == SOCK_VECTOR) {
  //  dt = DataType::VECTOR;
  //}
  // this->addInputSocket(dt, editorInput);

  // dt = DataType::VALUE;
  // if (editorOutput->type == SOCK_RGBA) {
  //  dt = DataType::COLOR;
  //}
  // if (editorOutput->type == SOCK_VECTOR) {
  //  dt = DataType::VECTOR;
  //}
  // this->addOutputSocket(dt, editorOutput);
}

void SocketProxyNode::convertToOperations(NodeConverter &converter,
                                          CompositorContext & /*context*/) const
{
  NodeOperationOutput *proxy_output = converter.addInputProxy(getInputSocket(0));
  converter.mapOutputSocket(getOutputSocket(), proxy_output);
}
