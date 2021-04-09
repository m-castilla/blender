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
 * Copyright 2013, Blender Foundation.
 */

#include "BLI_multi_value_map.hh"
#include "BLI_utildefines.h"

#include "COM_Converter.h"
#include "COM_Debug.h"
#include "COM_ExecutionSystem.h"
#include "COM_Node.h"
#include "COM_NodeConverter.h"
#include "COM_SocketProxyNode.h"

#include "COM_NodeOperation.h"
#include "COM_PreviewOperation.h"
#include "COM_SetColorOperation.h"
#include "COM_SetValueOperation.h"
#include "COM_SetVectorOperation.h"
#include "COM_SocketProxyOperation.h"
#include "COM_ViewerOperation.h"

#include "COM_NodeOperationBuilder.h" /* own include */

namespace blender::compositor {

NodeOperationBuilder::NodeOperationBuilder(const CompositorContext *context, bNodeTree *b_nodetree)
    : m_context(context), m_current_node(nullptr), m_active_viewer(nullptr)
{
  m_graph.from_bNodeTree(*context, b_nodetree);
}

void NodeOperationBuilder::convertToOperations(ExecutionSystem *system)
{
  /* interface handle for nodes */
  NodeConverter converter(this);

  for (Node *node : m_graph.nodes()) {
    m_current_node = node;

    DebugInfo::node_to_operations(node);
    node->convertToOperations(converter, *m_context);
  }

  m_current_node = nullptr;

  /* The input map constructed by nodes maps operation inputs to node inputs.
   * Inverting yields a map of node inputs to all connected operation inputs,
   * so multiple operations can use the same node input.
   */
  blender::MultiValueMap<NodeInput *, NodeOperationInput *> inverse_input_map;
  for (Map<NodeOperationInput *, NodeInput *>::MutableItem item : m_input_map.items()) {
    inverse_input_map.add(item.value, item.key);
  }

  for (const NodeGraph::Link &link : m_graph.links()) {
    NodeOutput *from = link.from;
    NodeInput *to = link.to;

    NodeOperationOutput *op_from = m_output_map.lookup_default(from, nullptr);

    const blender::Span<NodeOperationInput *> op_to_list = inverse_input_map.lookup(to);
    if (!op_from || op_to_list.is_empty()) {
      /* XXX allow this? error/debug message? */
      // BLI_assert(false);
      /* XXX note: this can happen with certain nodes (e.g. OutputFile)
       * which only generate operations in certain circumstances (rendering)
       * just let this pass silently for now ...
       */
      continue;
    }

    for (NodeOperationInput *op_to : op_to_list) {
      addLink(op_from, op_to);
    }
  }

  add_operation_input_constants();

  resolve_proxies();

  add_datatype_conversions();

  determineResolutions();

  /* links not available from here on */
  /* XXX make m_links a local variable to avoid confusion! */
  m_links.clear();

  prune_operations();

  /* ensure topological (link-based) order of nodes */
  /*sort_operations();*/ /* not needed yet */

  /* transfer resulting operations to the system */
  system->set_operations(m_operations);
}

void NodeOperationBuilder::addOperation(NodeOperation *operation)
{
  operation->set_id(m_operations.size());
  m_operations.append(operation);
  if (m_current_node) {
    operation->set_name(m_current_node->getbNode()->name);
  }
}

void NodeOperationBuilder::mapInputSocket(NodeInput *node_socket,
                                          NodeOperationInput *operation_socket)
{
  BLI_assert(m_current_node);
  BLI_assert(node_socket->getNode() == m_current_node);

  /* note: this maps operation sockets to node sockets.
   * for resolving links the map will be inverted first in convertToOperations,
   * to get a list of links for each node input socket.
   */
  m_input_map.add_new(operation_socket, node_socket);
}

void NodeOperationBuilder::mapOutputSocket(NodeOutput *node_socket,
                                           NodeOperationOutput *operation_socket)
{
  BLI_assert(m_current_node);
  BLI_assert(node_socket->getNode() == m_current_node);

  m_output_map.add_new(node_socket, operation_socket);
}

void NodeOperationBuilder::addLink(NodeOperationOutput *from, NodeOperationInput *to)
{
  if (to->isConnected()) {
    return;
  }

  m_links.append(Link(from, to));

  /* register with the input */
  to->setLink(from);
}

void NodeOperationBuilder::removeInputLink(NodeOperationInput *to)
{
  int index = 0;
  for (Link &link : m_links) {
    if (link.to() == to) {
      /* unregister with the input */
      to->setLink(nullptr);

      m_links.remove(index);
      return;
    }
    index++;
  }
}

PreviewOperation *NodeOperationBuilder::make_preview_operation() const
{
  BLI_assert(m_current_node);

  if (!(m_current_node->getbNode()->flag & NODE_PREVIEW)) {
    return nullptr;
  }
  /* previews only in the active group */
  if (!m_current_node->isInActiveGroup()) {
    return nullptr;
  }
  /* do not calculate previews of hidden nodes */
  if (m_current_node->getbNode()->flag & NODE_HIDDEN) {
    return nullptr;
  }

  bNodeInstanceHash *previews = m_context->getPreviewHash();
  if (previews) {
    PreviewOperation *operation = new PreviewOperation(m_context->getViewSettings(),
                                                       m_context->getDisplaySettings(),
                                                       m_current_node->getbNode()->preview_xsize,
                                                       m_current_node->getbNode()->preview_ysize);
    operation->setbNodeTree(m_context->getbNodeTree());
    operation->verifyPreview(previews, m_current_node->getInstanceKey());
    return operation;
  }

  return nullptr;
}

void NodeOperationBuilder::addPreview(NodeOperationOutput *output)
{
  PreviewOperation *operation = make_preview_operation();
  if (operation) {
    addOperation(operation);

    addLink(output, operation->getInputSocket(0));
  }
}

void NodeOperationBuilder::addNodeInputPreview(NodeInput *input)
{
  PreviewOperation *operation = make_preview_operation();
  if (operation) {
    addOperation(operation);

    mapInputSocket(input, operation->getInputSocket(0));
  }
}

void NodeOperationBuilder::registerViewer(ViewerOperation *viewer)
{
  if (m_active_viewer) {
    if (m_current_node->isInActiveGroup()) {
      /* deactivate previous viewer */
      m_active_viewer->setActive(false);

      m_active_viewer = viewer;
      viewer->setActive(true);
    }
  }
  else {
    if (m_current_node->getbNodeTree() == m_context->getbNodeTree()) {
      m_active_viewer = viewer;
      viewer->setActive(true);
    }
  }
}

/****************************
 **** Optimization Steps ****
 ****************************/

void NodeOperationBuilder::add_datatype_conversions()
{
  Vector<Link> convert_links;
  for (const Link &link : m_links) {
    /* proxy operations can skip data type conversion */
    NodeOperation *from_op = &link.from()->getOperation();
    NodeOperation *to_op = &link.to()->getOperation();
    if (!(from_op->get_flags().use_datatype_conversion ||
          to_op->get_flags().use_datatype_conversion)) {
      continue;
    }

    if (link.from()->getDataType() != link.to()->getDataType()) {
      convert_links.append(link);
    }
  }
  for (const Link &link : convert_links) {
    NodeOperation *converter = COM_convert_data_type(*link.from(), *link.to());
    if (converter) {
      addOperation(converter);

      removeInputLink(link.to());
      addLink(link.from(), converter->getInputSocket(0));
      addLink(converter->getOutputSocket(0), link.to());
    }
  }
}

void NodeOperationBuilder::add_operation_input_constants()
{
  /* Note: unconnected inputs cached first to avoid modifying
   *       m_operations while iterating over it
   */
  Vector<NodeOperationInput *> pending_inputs;
  for (NodeOperation *op : m_operations) {
    for (int k = 0; k < op->getNumberOfInputSockets(); ++k) {
      NodeOperationInput *input = op->getInputSocket(k);
      if (!input->isConnected()) {
        pending_inputs.append(input);
      }
    }
  }
  for (NodeOperationInput *input : pending_inputs) {
    add_input_constant_value(input, m_input_map.lookup_default(input, nullptr));
  }
}

void NodeOperationBuilder::add_input_constant_value(NodeOperationInput *input,
                                                    const NodeInput *node_input)
{
  switch (input->getDataType()) {
    case DataType::Value: {
      float value;
      if (node_input && node_input->getbNodeSocket()) {
        value = node_input->getEditorValueFloat();
      }
      else {
        value = 0.0f;
      }

      SetValueOperation *op = new SetValueOperation();
      op->setValue(value);
      addOperation(op);
      addLink(op->getOutputSocket(), input);
      break;
    }
    case DataType::Color: {
      float value[4];
      if (node_input && node_input->getbNodeSocket()) {
        node_input->getEditorValueColor(value);
      }
      else {
        zero_v4(value);
      }

      SetColorOperation *op = new SetColorOperation();
      op->setChannels(value);
      addOperation(op);
      addLink(op->getOutputSocket(), input);
      break;
    }
    case DataType::Vector: {
      float value[3];
      if (node_input && node_input->getbNodeSocket()) {
        node_input->getEditorValueVector(value);
      }
      else {
        zero_v3(value);
      }

      SetVectorOperation *op = new SetVectorOperation();
      op->setVector(value);
      addOperation(op);
      addLink(op->getOutputSocket(), input);
      break;
    }
  }
}

void NodeOperationBuilder::resolve_proxies()
{
  Vector<Link> proxy_links;
  for (const Link &link : m_links) {
    /* don't replace links from proxy to proxy, since we may need them for replacing others! */
    if (link.from()->getOperation().get_flags().is_proxy_operation &&
        !link.to()->getOperation().get_flags().is_proxy_operation) {
      proxy_links.append(link);
    }
  }

  for (const Link &link : proxy_links) {
    NodeOperationInput *to = link.to();
    NodeOperationOutput *from = link.from();
    do {
      /* walk upstream bypassing the proxy operation */
      from = from->getOperation().getInputSocket(0)->getLink();
    } while (from && from->getOperation().get_flags().is_proxy_operation);

    removeInputLink(to);
    /* we may not have a final proxy input link,
     * in that case it just gets dropped
     */
    if (from) {
      addLink(from, to);
    }
  }
}

void NodeOperationBuilder::determineResolutions()
{
  /* determine all resolutions of the operations (Width/Height) */
  for (NodeOperation *op : m_operations) {
    if (op->isOutputOperation(m_context->isRendering()) && !op->get_flags().is_preview_operation) {
      unsigned int resolution[2] = {0, 0};
      unsigned int preferredResolution[2] = {0, 0};
      op->determineResolution(resolution, preferredResolution);
      op->setResolution(resolution);
    }
  }

  for (NodeOperation *op : m_operations) {
    if (op->isOutputOperation(m_context->isRendering()) && op->get_flags().is_preview_operation) {
      unsigned int resolution[2] = {0, 0};
      unsigned int preferredResolution[2] = {0, 0};
      op->determineResolution(resolution, preferredResolution);
      op->setResolution(resolution);
    }
  }

  /* add convert resolution operations when needed */
  {
    Vector<Link> convert_links;
    for (const Link &link : m_links) {
      if (link.to()->getResizeMode() != ResizeMode::None) {
        NodeOperation &from_op = link.from()->getOperation();
        NodeOperation &to_op = link.to()->getOperation();
        if (from_op.getWidth() != to_op.getWidth() || from_op.getHeight() != to_op.getHeight()) {
          convert_links.append(link);
        }
      }
    }
    for (const Link &link : convert_links) {
      COM_convert_resolution(*this, link.from(), link.to());
    }
  }
}

using Tags = std::set<NodeOperation *>;

static void find_reachable_operations_recursive(Tags &reachable, NodeOperation *op)
{
  if (reachable.find(op) != reachable.end()) {
    return;
  }
  reachable.insert(op);

  for (int i = 0; i < op->getNumberOfInputSockets(); i++) {
    NodeOperationInput *input = op->getInputSocket(i);
    if (input->isConnected()) {
      find_reachable_operations_recursive(reachable, &input->getLink()->getOperation());
    }
  }
}

void NodeOperationBuilder::prune_operations()
{
  Tags reachable;
  for (NodeOperation *op : m_operations) {
    /* output operations are primary executed operations */
    if (op->isOutputOperation(m_context->isRendering())) {
      find_reachable_operations_recursive(reachable, op);
    }
  }

  /* delete unreachable operations */
  Vector<NodeOperation *> reachable_ops;
  for (NodeOperation *op : m_operations) {
    if (reachable.find(op) != reachable.end()) {
      reachable_ops.append(op);
    }
    else {
      delete op;
    }
  }
  /* finally replace the operations list with the pruned list */
  m_operations = reachable_ops;
}

/* topological (depth-first) sorting of operations */
static void sort_operations_recursive(Vector<NodeOperation *> &sorted,
                                      Tags &visited,
                                      NodeOperation *op)
{
  if (visited.find(op) != visited.end()) {
    return;
  }
  visited.insert(op);

  for (int i = 0; i < op->getNumberOfInputSockets(); i++) {
    NodeOperationInput *input = op->getInputSocket(i);
    if (input->isConnected()) {
      sort_operations_recursive(sorted, visited, &input->getLink()->getOperation());
    }
  }

  sorted.append(op);
}

void NodeOperationBuilder::sort_operations()
{
  Vector<NodeOperation *> sorted;
  sorted.reserve(m_operations.size());
  Tags visited;

  for (NodeOperation *operation : m_operations) {
    sort_operations_recursive(sorted, visited, operation);
  }

  m_operations = sorted;
}

/** Create a graphviz representation of the NodeOperationBuilder. */
std::ostream &operator<<(std::ostream &os, const NodeOperationBuilder &builder)
{
  os << "# Builder start\n";
  os << "digraph  G {\n";
  os << "    rankdir=LR;\n";
  os << "    node [shape=box];\n";
  for (const NodeOperation *operation : builder.get_operations()) {
    os << "    op" << operation->get_id() << " [label=\"" << *operation << "\"];\n";
  }

  os << "\n";
  for (const NodeOperationBuilder::Link &link : builder.get_links()) {
    os << "    op" << link.from()->getOperation().get_id() << " -> op"
       << link.to()->getOperation().get_id() << ";\n";
  }

  os << "}\n";
  os << "# Builder end\n";
  return os;
}

std::ostream &operator<<(std::ostream &os, const NodeOperationBuilder::Link &link)
{
  os << link.from()->getOperation().get_id() << " -> " << link.to()->getOperation().get_id();
  return os;
}

}  // namespace blender::compositor
