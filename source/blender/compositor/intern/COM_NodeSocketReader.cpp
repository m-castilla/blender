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

#include "BLI_assert.h"

#include "COM_AntiAliasOperation.h"
#include "COM_BufferManager.h"
#include "COM_ComputeManager.h"
#include "COM_ExecutionManager.h"
#include "COM_MixOperation.h"
#include "COM_Node.h"
#include "COM_NodeOperation.h"
#include "COM_NodeSocketReader.h" /* own include */
#include "COM_PixelsUtil.h"
#include "COM_SetColorOperation.h"
#include "COM_TranslateOperation.h"

using namespace std::placeholders;
/*******************
 **** NodeOperation ****
 *******************/
uint NodeSocketReader::s_order_counter = 0;

NodeSocketReader::NodeSocketReader()
{
  this->m_order = s_order_counter;
  s_order_counter++;
  this->m_mainInputSocketIndex = 0;
  this->m_width = 0;
  this->m_height = 0;
  this->m_isResolutionSet = false;
  this->m_btree = NULL;
}

NodeSocketReader::~NodeSocketReader()
{
  while (!this->m_outputs.empty()) {
    delete (this->m_outputs.back());
    this->m_outputs.pop_back();
  }
  while (!this->m_inputs.empty()) {
    delete (this->m_inputs.back());
    this->m_inputs.pop_back();
  }
}

NodeOperationInput *NodeSocketReader::getInputSocket(int index) const
{
  BLI_assert(index < m_inputs.size());
  return m_inputs[index];
}

NodeOperationOutput *NodeSocketReader::getOutputSocket(int index) const
{
  BLI_assert(index < m_outputs.size());
  return m_outputs[index];
}

DataType NodeSocketReader::getOutputDataType() const
{
  return getOutputSocket()->getDataType();
}

int NodeSocketReader::getOutputNChannels() const
{
  return getOutputSocket()->getNChannels();
}

NodeOperationInput *NodeSocketReader::addInputSocket(SocketType socket_type,
                                                     InputResizeMode resize_mode)
{
  NodeOperationInput *socket = new NodeOperationInput(
      (NodeOperation *)this, socket_type, resize_mode);
  m_inputs.push_back(socket);
  return socket;
}

NodeOperationOutput *NodeSocketReader::addOutputSocket(SocketType socket_type)
{
  NodeOperationOutput *socket = new NodeOperationOutput((NodeOperation *)this, socket_type);
  m_outputs.push_back(socket);
  return socket;
}

void NodeSocketReader::determineResolution(int resolution[2],
                                           int preferredResolution[2],
                                           bool setResolution)
{

  /* Look for a valid resolution of any input. First looking at the main input socket. This doesn't
   * set resolution on inputs yet */
  bool is_local_preferred_set = false;
  int local_preferred[2] = {preferredResolution[0], preferredResolution[1]};
  if (m_inputs.size() > 0) {
    NodeOperationInput *input = m_inputs[m_mainInputSocketIndex];
    // bool is_translate = typeid(*this) == typeid(TranslateOperation);
    if (input->isConnected()) {
      int temp[2] = {0, 0};
      input->determineResolution(temp, local_preferred, false);
      if (temp[0] > 0 && temp[1] > 0) {
        local_preferred[0] = temp[0];
        local_preferred[1] = temp[1];
        is_local_preferred_set = true;
      }
    }

    if (!is_local_preferred_set) {
      for (int index = 0; index < m_inputs.size(); index++) {
        if (index != m_mainInputSocketIndex) {
          input = m_inputs[index];
          if (input->isConnected()) {
            int temp[2] = {0, 0};
            input->determineResolution(temp, local_preferred, false);
            if (temp[0] > 0 && temp[1] > 0) {
              local_preferred[0] = temp[0];
              local_preferred[1] = temp[1];
              break;
            }
          }
        }
      }
    }

    /* Determine and set inputs resolutions taking into account our local preferred resolution */
    bool is_local_res_set = false;
    input = m_inputs[m_mainInputSocketIndex];
    if (input->isConnected()) {
      input->determineResolution(resolution, local_preferred, setResolution);
      if (resolution[0] > 0 && resolution[1] > 0) {
        is_local_res_set = true;
      }
    }
    for (int index = 0; index < m_inputs.size(); index++) {
      if (index != m_mainInputSocketIndex) {
        input = m_inputs[index];
        if (input->isConnected()) {
          if (is_local_res_set) {
            int temp[2] = {0, 0};
            input->determineResolution(temp, local_preferred, setResolution);
          }
          else {
            input->determineResolution(resolution, local_preferred, setResolution);
            if (resolution[0] > 0 && resolution[1] > 0) {
              is_local_res_set = true;
            }
          }
        }
      }
    }
  }

  if (resolution[0] <= 0 || resolution[1] <= 0) {
    resolution[0] = local_preferred[0];
    resolution[1] = local_preferred[1];
  }
}
void NodeSocketReader::setMainInputSocketIndex(int index)
{
  this->m_mainInputSocketIndex = index;
}
void NodeSocketReader::initExecution()
{
}

void NodeSocketReader::deinitExecution()
{
  /* pass */
}

NodeOperation *NodeSocketReader::getInputOperation(int inputSocketIndex) const
{
  NodeOperationInput *input = getInputSocket(inputSocketIndex);
  if (input && input->isConnected()) {
    return input->getLink()->getOperation();
  }
  else {
    return NULL;
  }
}

void NodeSocketReader::getConnectedInputSockets(Inputs *sockets) const
{
  for (Inputs::const_iterator it = m_inputs.begin(); it != m_inputs.end(); ++it) {
    NodeOperationInput *input = *it;
    if (input->isConnected()) {
      sockets->push_back(input);
    }
  }
}

/*****************
 **** OpInput ****
 *****************/

NodeOperationInput::NodeOperationInput(NodeOperation *op,
                                       SocketType socket_type,
                                       InputResizeMode resizeMode)
    : m_operation(op),
      m_socket_type(socket_type),
      m_resizeMode(resizeMode),
      m_link(nullptr),
      m_has_user_link(true)
{
}

DataType NodeOperationInput::getDataType() const
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
      BLI_assert(!"Non implemented SocketType");
      return (DataType)0;
  }
}
bool NodeOperationInput::hasDataType() const
{
  if (m_socket_type == SocketType::DYNAMIC) {
    return m_link ? m_link->hasDataType() : false;
  }
  else {
    return true;
  }
}

int NodeOperationInput::getNChannels() const
{
  return PixelsUtil::getNChannels(getDataType());
}

NodeOperation *NodeOperationInput::getLinkedOp()
{
  if (isConnected()) {
    return m_link->getOperation();
  }
  else {
    return nullptr;
  }
}

void NodeOperationInput::determineResolution(int resolution[2],
                                             int preferredResolution[2],
                                             bool setResolution)
{
  if (m_link) {
    m_link->determineResolution(resolution, preferredResolution, setResolution);
  }
}

/******************
 **** OpOutput ****
 ******************/

NodeOperationOutput::NodeOperationOutput(NodeOperation *op, SocketType socket_type)
    : m_operation(op), m_socket_type(socket_type), m_bOutputSocket(NULL)
{
}

DataType NodeOperationOutput::getDataType() const
{
  switch (m_socket_type) {
    case SocketType::VALUE:
      return DataType::VALUE;
    case SocketType::VECTOR:
      return DataType::VECTOR;
    case SocketType::COLOR:
      return DataType::COLOR;
    case SocketType::DYNAMIC:
      return m_operation->getMainInputSocket()->getDataType();
    default:
      BLI_assert(!"Non implemented SocketType");
      return (DataType)0;
  }
}

bool NodeOperationOutput::hasDataType() const
{
  if (m_socket_type == SocketType::DYNAMIC) {
    return m_operation->getNumberOfInputSockets() > 0 ?
               m_operation->getMainInputSocket()->hasDataType() :
               false;
  }
  else {
    return true;
  }
}

int NodeOperationOutput::getNChannels() const
{
  return PixelsUtil::getNChannels(getDataType());
}

void NodeOperationOutput::determineResolution(int resolution[2],
                                              int preferredResolution[2],
                                              bool setResolution)
{
  NodeOperation *operation = getOperation();
  if (operation->isResolutionSet()) {
    resolution[0] = operation->getWidth();
    resolution[1] = operation->getHeight();
  }
  else {
    operation->determineResolution(resolution, preferredResolution, setResolution);
    if (setResolution) {
      operation->setResolution(resolution);
    }
  }
}

void NodeOperationOutput::setAsNodeOutput(NodeOutput *nodeOutput)
{
  m_bOutputSocket = nodeOutput->getbNodeSocket();
  BLI_assert(m_bOutputSocket);
}
