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

#include <cstdio>
#include <typeinfo>

#include "COM_ExecutionSystem.h"
#include "COM_defines.h"

#include "COM_NodeOperation.h" /* own include */

namespace blender::compositor {

/*******************
 **** NodeOperation ****
 *******************/

NodeOperation::NodeOperation()
{
  this->m_resolutionInputSocketIndex = 0;
  this->m_width = 0;
  this->m_height = 0;
  this->m_btree = nullptr;
}

NodeOperationOutput *NodeOperation::getOutputSocket(unsigned int index)
{
  return &m_outputs[index];
}

NodeOperationInput *NodeOperation::getInputSocket(unsigned int index)
{
  return &m_inputs[index];
}

void NodeOperation::addInputSocket(DataType datatype, ResizeMode resize_mode)
{
  m_inputs.append(NodeOperationInput(this, datatype, resize_mode));
}

void NodeOperation::addOutputSocket(DataType datatype)
{
  m_outputs.append(NodeOperationOutput(this, datatype));
}

void NodeOperation::determineResolution(unsigned int resolution[2],
                                        unsigned int preferredResolution[2])
{
  if (m_resolutionInputSocketIndex < m_inputs.size()) {
    NodeOperationInput &input = m_inputs[m_resolutionInputSocketIndex];
    input.determineResolution(resolution, preferredResolution);
  }
  unsigned int temp2[2] = {resolution[0], resolution[1]};

  unsigned int temp[2];
  for (unsigned int index = 0; index < m_inputs.size(); index++) {
    if (index == this->m_resolutionInputSocketIndex) {
      continue;
    }
    NodeOperationInput &input = m_inputs[index];
    if (input.isConnected()) {
      input.determineResolution(temp, temp2);
    }
  }
}

void NodeOperation::setResolutionInputSocketIndex(unsigned int index)
{
  this->m_resolutionInputSocketIndex = index;
}
void NodeOperation::initExecution()
{
  /* pass */
}

void NodeOperation::initMutex()
{
  BLI_mutex_init(&this->m_mutex);
}

void NodeOperation::lockMutex()
{
  BLI_mutex_lock(&this->m_mutex);
}

void NodeOperation::unlockMutex()
{
  BLI_mutex_unlock(&this->m_mutex);
}

void NodeOperation::deinitMutex()
{
  BLI_mutex_end(&this->m_mutex);
}

void NodeOperation::deinitExecution()
{
  /* pass */
}
SocketReader *NodeOperation::getInputSocketReader(unsigned int inputSocketIndex)
{
  return this->getInputSocket(inputSocketIndex)->getReader();
}

NodeOperation *NodeOperation::getInputOperation(unsigned int inputSocketIndex)
{
  NodeOperationInput *input = getInputSocket(inputSocketIndex);
  if (input && input->isConnected()) {
    return &input->getLink()->getOperation();
  }

  return nullptr;
}

void NodeOperation::determineRectsToRender(const rcti &render_rect, OutputManager *output_man)
{
  if (!output_man->isRenderRegistered(this, render_rect)) {
    output_man->registerRender(this, render_rect);

    int n_inputs = getNumberOfInputSockets();
    for (int i = 0; i < n_inputs; i++) {
      auto input_op = getInputOperation(i);
      auto input_area = getInputAreaOfInterest(i, render_rect);
      input_op->determineRectsToRender(input_area, output_man);
    }
  }
}

void NodeOperation::registerReads(OutputManager *output_man)
{
  if (!output_man->hasRegisteredReads(this)) {
    int n_inputs = getNumberOfInputSockets();
    for (int i = 0; i < n_inputs; i++) {
      auto input_op = getInputOperation(i);
      input_op->registerReads(output_man);
      output_man->registerRead(input_op);
    }
  }
}

void NodeOperation::renderPixels(ExecutionSystem *exec_system)
{
  OutputManager *output_man = exec_system->getOutputManager();
  if (!output_man->isOutputRendered(this)) {
    /* ensure inputs are rendered */
    int n_inputs_sockets = getNumberOfInputSockets();
    blender::Vector<NodeOperation *> input_ops;
    for (int i = 0; i < n_inputs_sockets; i++) {
      auto input_op = getInputOperation(i);
      if (input_op) {
        if (!output_man->isOutputRendered(input_op)) {
          input_op->renderPixels(exec_system);
        }
        input_ops.append(input_op);
      }
    }

    initExecution();
    auto render_rects = output_man->getRectsToRender(this);
    bool has_output = m_outputs.size() > 0;
    auto data_type = has_output ? getOutputSocket(0)->getDataType() : DataType::Color;
    bool has_gpu_support = exec_system->hasGpuSupport() && get_flags().open_cl;
    if (has_gpu_support) {
      /* get inputs as gpu buffers */
      blender::Vector<const GPUBuffer *> inputs_bufs;
      for (auto input_op : input_ops) {
        inputs_bufs.append(output_man->getOutputGPU(input_op));
      }

      /* gpu render */
      auto gpu_man = exec_system->getGPUBufferManager();
      GPUBufferUniquePtr output_buf;
      if (has_output) {
        output_buf = gpu_man->takeImageBuffer(
            data_type, getWidth(), getHeight(), get_flags().is_set_operation);
      }
      for (const rcti &render_rect : render_rects) {
        execPixelsGPU(render_rect, *output_buf.get(), inputs_bufs.as_span(), exec_system);
      }
      output_man->giveRenderedOutput(this, std::move(output_buf));
    }
    else {
      /* get inputs as cpu buffers */
      blender::Vector<const CPUBuffer<float> *> inputs_bufs;
      for (auto input_op : input_ops) {
        inputs_bufs.append(output_man->getOutputCPU(input_op));
      }

      /* cpu render */
      auto cpu_man = exec_system->getCPUBufferManager();
      CPUBufferUniquePtr<float> output_buf;
      if (has_output) {
        output_buf = cpu_man->takeImageBuffer(
            data_type, getWidth(), getHeight(), get_flags().is_set_operation);
      }
      for (const rcti &render_rect : render_rects) {
        execPixelsCPU(render_rect, *output_buf.get(), inputs_bufs.as_span(), exec_system);
      }
      output_man->giveRenderedOutput(this, std::move(output_buf));
    }
    deinitExecution();

    /* report inputs reads so that buffers may be freed/recycled when total reads reached */
    for (auto input_op : input_ops) {
      output_man->reportRead(input_op);
    }

    exec_system->reportOperationEnd();
  }
}

bool NodeOperation::determineDependingAreaOfInterest(rcti *input,
                                                     ReadBufferOperation *readOperation,
                                                     rcti *output)
{
  if (m_inputs.size() == 0) {
    BLI_rcti_init(output, input->xmin, input->xmax, input->ymin, input->ymax);
    return false;
  }

  rcti tempOutput;
  bool first = true;
  for (int i = 0; i < getNumberOfInputSockets(); i++) {
    NodeOperation *inputOperation = this->getInputOperation(i);
    if (inputOperation &&
        inputOperation->determineDependingAreaOfInterest(input, readOperation, &tempOutput)) {
      if (first) {
        output->xmin = tempOutput.xmin;
        output->ymin = tempOutput.ymin;
        output->xmax = tempOutput.xmax;
        output->ymax = tempOutput.ymax;
        first = false;
      }
      else {
        output->xmin = MIN2(output->xmin, tempOutput.xmin);
        output->ymin = MIN2(output->ymin, tempOutput.ymin);
        output->xmax = MAX2(output->xmax, tempOutput.xmax);
        output->ymax = MAX2(output->ymax, tempOutput.ymax);
      }
    }
  }
  return !first;
}

/*****************
 **** OpInput ****
 *****************/

NodeOperationInput::NodeOperationInput(NodeOperation *op, DataType datatype, ResizeMode resizeMode)
    : m_operation(op), m_datatype(datatype), m_resizeMode(resizeMode), m_link(nullptr)
{
}

SocketReader *NodeOperationInput::getReader()
{
  if (isConnected()) {
    return &m_link->getOperation();
  }

  return nullptr;
}

void NodeOperationInput::determineResolution(unsigned int resolution[2],
                                             unsigned int preferredResolution[2])
{
  if (m_link) {
    m_link->determineResolution(resolution, preferredResolution);
  }
}

/******************
 **** OpOutput ****
 ******************/

NodeOperationOutput::NodeOperationOutput(NodeOperation *op, DataType datatype)
    : m_operation(op), m_datatype(datatype)
{
}

void NodeOperationOutput::determineResolution(unsigned int resolution[2],
                                              unsigned int preferredResolution[2])
{
  NodeOperation &operation = getOperation();
  if (operation.get_flags().is_resolution_set) {
    resolution[0] = operation.getWidth();
    resolution[1] = operation.getHeight();
  }
  else {
    operation.determineResolution(resolution, preferredResolution);
    operation.setResolution(resolution);
  }
}

std::ostream &operator<<(std::ostream &os, const NodeOperationFlags &node_operation_flags)
{
  if (node_operation_flags.complex) {
    os << "complex,";
  }
  if (node_operation_flags.open_cl) {
    os << "open_cl,";
  }
  if (node_operation_flags.single_threaded) {
    os << "single_threaded,";
  }
  if (node_operation_flags.use_render_border) {
    os << "render_border,";
  }
  if (node_operation_flags.use_viewer_border) {
    os << "view_border,";
  }
  if (node_operation_flags.is_resolution_set) {
    os << "resolution_set,";
  }
  if (node_operation_flags.is_set_operation) {
    os << "set_operation,";
  }
  if (node_operation_flags.is_write_buffer_operation) {
    os << "write_buffer,";
  }
  if (node_operation_flags.is_read_buffer_operation) {
    os << "read_buffer,";
  }
  if (node_operation_flags.is_proxy_operation) {
    os << "proxy,";
  }
  if (node_operation_flags.is_viewer_operation) {
    os << "viewer,";
  }
  if (node_operation_flags.is_preview_operation) {
    os << "preview,";
  }
  if (!node_operation_flags.use_datatype_conversion) {
    os << "no_conversion,";
  }

  return os;
}

std::ostream &operator<<(std::ostream &os, const NodeOperation &node_operation)
{
  NodeOperationFlags flags = node_operation.get_flags();
  os << "NodeOperation(";
  os << "id=" << node_operation.get_id();
  if (!node_operation.get_name().empty()) {
    os << ",name=" << node_operation.get_name();
  }
  os << ",flags={" << flags << "}";
  os << ")";

  return os;
}

}  // namespace blender::compositor
