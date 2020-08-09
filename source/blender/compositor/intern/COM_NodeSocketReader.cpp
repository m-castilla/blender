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
#include "COM_CompositorContext.h"
#include "COM_ComputeManager.h"
#include "COM_ExecutionManager.h"
#include "COM_GlobalManager.h"
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

NodeOperationInput *NodeSocketReader::addInputSocket(SocketType socket_type)
{
  return addInputSocket(socket_type, GlobalMan->getContext()->getDefaultInputResizeMode());
}

NodeOperationOutput *NodeSocketReader::addOutputSocket(SocketType socket_type)
{
  NodeOperationOutput *socket = new NodeOperationOutput((NodeOperation *)this, socket_type);
  m_outputs.push_back(socket);
  return socket;
}

void NodeSocketReader::determineResolution(int resolution[2],
                                           int preferredResolution[2],
                                           DetermineResolutionMode mode,
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
      input->determineResolution(temp, local_preferred, DetermineResolutionMode::FromInput, false);
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
            input->determineResolution(
                temp, local_preferred, DetermineResolutionMode::FromInput, false);
            if (temp[0] > 0 && temp[1] > 0) {
              local_preferred[0] = temp[0];
              local_preferred[1] = temp[1];
              break;
            }
          }
        }
      }
    }

    if (mode == DetermineResolutionMode::FromOutput) {
      if (preferredResolution[0] > 0 && preferredResolution[1] > 0 && local_preferred[0] > 0 &&
          local_preferred[1] > 0) {
        float scaleX = (float)preferredResolution[0] / local_preferred[0];
        float scaleY = (float)preferredResolution[1] / local_preferred[1];
        float scale = scaleX < scaleY ? scaleY : scaleX;
        if (scale < 1.0f) {
          local_preferred[0] *= scale;
          local_preferred[1] *= scale;
        }
      }
    }

    /* Determine and set inputs resolutions taking into account our local preferred resolution */
    bool is_local_res_set = false;
    input = m_inputs[m_mainInputSocketIndex];
    if (input->isConnected()) {
      input->determineResolution(resolution, local_preferred, mode, setResolution);
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
            input->determineResolution(temp, local_preferred, mode, setResolution);
          }
          else {
            input->determineResolution(resolution, local_preferred, mode, setResolution);
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
  m_determined_widths.clear();
  m_determined_heights.clear();
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

/**
 * \brief set the resolution
 * \param resolution: the resolution to set
 */
void NodeSocketReader::setResolution(int width, int height)
{
  if (!isResolutionSet()) {
    this->m_width = width;
    this->m_height = height;
    this->m_isResolutionSet = true;
  }
}

void NodeSocketReader::addDeterminedResolution(int resolution[2])
{
  m_determined_widths.push_back(resolution[0]);
  m_determined_heights.push_back(resolution[1]);
}

void NodeSocketReader::setBestDeterminedResolution()
{
  if (!isResolutionSet()) {
    BLI_assert(m_determined_widths.size() == m_determined_heights.size());
    int n_items = m_determined_widths.size();
    int res_w = 0;
    int res_h = 0;
    int area = 0;
    for (int idx = 0; idx < n_items; idx++) {
      int new_res_w = m_determined_widths[idx];
      int new_res_h = m_determined_heights[idx];
      int new_area = new_res_w * new_res_h;
      if (new_area > area) {
        res_w = new_res_w;
        res_h = new_res_h;
        area = new_area;
      }
    }
    setResolution(res_w, res_h);
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
                                             DetermineResolutionMode mode,
                                             bool setResolution)
{
  if (m_link) {
    m_link->determineResolution(resolution, preferredResolution, mode, setResolution);
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
                                              DetermineResolutionMode mode,
                                              bool setResolution)
{
  NodeOperation *operation = getOperation();
  if (operation->isResolutionSet()) {
    resolution[0] = operation->getWidth();
    resolution[1] = operation->getHeight();
  }
  else {
    operation->determineResolution(resolution, preferredResolution, mode, setResolution);
    if (setResolution) {
      switch (mode) {
        case DetermineResolutionMode::FromInput:
          operation->setResolution(resolution[0], resolution[1]);
          break;
        case DetermineResolutionMode::FromOutput:
          operation->addDeterminedResolution(resolution);
          break;
        default:
          BLI_assert(!"Non implemented DetermineResolutionMode");
      }
    }
  }
}

void NodeOperationOutput::setAsNodeOutput(NodeOutput *nodeOutput)
{
  m_bOutputSocket = nodeOutput->getbNodeSocket();
  BLI_assert(m_bOutputSocket);
}
