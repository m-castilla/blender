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

#include <string.h>

#include "BKE_node.h"

#include "RNA_access.h"

#include "COM_ExecutionSystem.h"
#include "COM_NodeOperation.h"
#include "COM_TranslateOperation.h"

#include "COM_SocketProxyNode.h"

#include "COM_defines.h"

#include "COM_Node.h" /* own include */

/**************
 **** Node ****
 **************/

static SocketType getSocketType(bNodeSocket *socket)
{
  SocketType st = SocketType::DYNAMIC;
  if (socket->type == SOCK_RGBA) {
    st = SocketType::COLOR;
  }
  else if (socket->type == SOCK_VECTOR) {
    st = SocketType::VECTOR;
  }
  else if (socket->type == SOCK_FLOAT) {
    st = SocketType::VALUE;
  }
  return st;
}

Node::Node(bNode *editorNode, bool create_sockets)
    : m_editorNodeTree(NULL),
      m_editorNode(editorNode),
      m_inActiveGroup(false),
      m_instanceKey(NODE_INSTANCE_KEY_NONE)
{
  if (create_sockets) {
    bNodeSocket *input = (bNodeSocket *)editorNode->inputs.first;
    while (input != NULL) {
      this->addInputSocket(getSocketType(input), input);
      input = input->next;
    }
    bNodeSocket *output = (bNodeSocket *)editorNode->outputs.first;
    while (output != NULL) {
      this->addOutputSocket(getSocketType(output), output);
      output = output->next;
    }
  }
  m_main_input_socket_idx = 0;
}

Node::~Node()
{
  while (!this->m_outputsockets.empty()) {
    delete (this->m_outputsockets.back());
    this->m_outputsockets.pop_back();
  }
  while (!this->m_inputsockets.empty()) {
    delete (this->m_inputsockets.back());
    this->m_inputsockets.pop_back();
  }
}

void Node::addInputSocket(SocketType socket_type)
{
  this->addInputSocket(socket_type, NULL);
}

void Node::addInputSocket(SocketType socket_type, bNodeSocket *bSocket)
{
  NodeInput *socket = new NodeInput(this, bSocket, socket_type);
  this->m_inputsockets.push_back(socket);
}

void Node::addOutputSocket(SocketType socket_type)
{
  this->addOutputSocket(socket_type, NULL);
}
void Node::addOutputSocket(SocketType socket_type, bNodeSocket *bSocket)
{
  NodeOutput *socket = new NodeOutput(this, bSocket, socket_type);
  this->m_outputsockets.push_back(socket);
}

NodeOutput *Node::getOutputSocket(int index) const
{
  BLI_assert(index < this->m_outputsockets.size());
  return this->m_outputsockets[index];
}

NodeInput *Node::getInputSocket(int index) const
{
  BLI_assert(index < this->m_inputsockets.size());
  return this->m_inputsockets[index];
}

NodeInput *Node::getMainInputSocket() const
{
  return getInputSocket(m_main_input_socket_idx);
}

bNodeSocket *Node::getEditorInputSocket(int editorNodeInputSocketIndex)
{
  bNodeSocket *bSock = (bNodeSocket *)this->getbNode()->inputs.first;
  int index = 0;
  while (bSock != NULL) {
    if (index == editorNodeInputSocketIndex) {
      return bSock;
    }
    index++;
    bSock = bSock->next;
  }
  return NULL;
}
bNodeSocket *Node::getEditorOutputSocket(int editorNodeOutputSocketIndex)
{
  bNodeSocket *bSock = (bNodeSocket *)this->getbNode()->outputs.first;
  int index = 0;
  while (bSock != NULL) {
    if (index == editorNodeOutputSocketIndex) {
      return bSock;
    }
    index++;
    bSock = bSock->next;
  }
  return NULL;
}

/*******************
 **** NodeInput ****
 *******************/

NodeInput::NodeInput(Node *node, bNodeSocket *b_socket, SocketType socket_type)
    : m_node(node), m_editorSocket(b_socket), m_socket_type(socket_type), m_link(NULL)
{
}

void NodeInput::setLink(NodeOutput *link)
{
  m_link = link;
}

DataType NodeInput::getDataType() const
{
  switch (m_socket_type) {
    case SocketType::VALUE:
      return DataType::VALUE;
    case SocketType::VECTOR:
      return DataType::VECTOR;
    case SocketType::COLOR:
      return DataType::COLOR;
    case SocketType::DYNAMIC:
      // A dynamic input requires a linked input
      BLI_assert(m_link);
      return m_link->getDataType();
    default:
      BLI_assert(!"None implemented SocketType");
      return (DataType)0;
  }
}

float NodeInput::getEditorValueFloat()
{
  PointerRNA ptr;
  RNA_pointer_create((ID *)getNode()->getbNodeTree(), &RNA_NodeSocket, getbNodeSocket(), &ptr);
  return RNA_float_get(&ptr, "default_value");
}

void NodeInput::getEditorValueColor(float *value)
{
  PointerRNA ptr;
  RNA_pointer_create((ID *)getNode()->getbNodeTree(), &RNA_NodeSocket, getbNodeSocket(), &ptr);
  return RNA_float_get_array(&ptr, "default_value", value);
}

void NodeInput::getEditorValueVector(float *value)
{
  PointerRNA ptr;
  RNA_pointer_create((ID *)getNode()->getbNodeTree(), &RNA_NodeSocket, getbNodeSocket(), &ptr);
  return RNA_float_get_array(&ptr, "default_value", value);
}

/********************
 **** NodeOutput ****
 ********************/

NodeOutput::NodeOutput(Node *node, bNodeSocket *b_socket, SocketType socket_type)
    : m_node(node), m_editorSocket(b_socket), m_socket_type(socket_type)
{
}

DataType NodeOutput::getDataType() const
{
  switch (m_socket_type) {
    case SocketType::VALUE:
      return DataType::VALUE;
    case SocketType::VECTOR:
      return DataType::VECTOR;
    case SocketType::COLOR:
      return DataType::COLOR;
    case SocketType::DYNAMIC:
      // A main input socket must be set when a node has a dynamic output socket
      return m_node->getMainInputSocket()->getDataType();
    default:
      BLI_assert(!"Non implemented SocketType");
      return (DataType)0;
  }
}

float NodeOutput::getEditorValueFloat()
{
  PointerRNA ptr;
  RNA_pointer_create((ID *)getNode()->getbNodeTree(), &RNA_NodeSocket, getbNodeSocket(), &ptr);
  return RNA_float_get(&ptr, "default_value");
}

void NodeOutput::getEditorValueColor(float *value)
{
  PointerRNA ptr;
  RNA_pointer_create((ID *)getNode()->getbNodeTree(), &RNA_NodeSocket, getbNodeSocket(), &ptr);
  return RNA_float_get_array(&ptr, "default_value", value);
}

void NodeOutput::getEditorValueVector(float *value)
{
  PointerRNA ptr;
  RNA_pointer_create((ID *)getNode()->getbNodeTree(), &RNA_NodeSocket, getbNodeSocket(), &ptr);
  return RNA_float_get_array(&ptr, "default_value", value);
}
