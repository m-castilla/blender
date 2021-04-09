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

#include <cstring>

#include "DNA_node_types.h"

#include "BKE_node.h"

#include "COM_NodeOperation.h"
#include "COM_NodeOperationBuilder.h"

#include "COM_ConvertOperation.h"
#include "COM_Converter.h"
#include "COM_DirectionalBlurNode.h"
#include "COM_ExecutionSystem.h"
#include "COM_GammaNode.h"
#include "COM_ImageNode.h"
#include "COM_ScaleOperation.h"
#include "COM_SetValueOperation.h"
#include "COM_TranslateOperation.h"
#include "COM_ViewerNode.h"

namespace blender::compositor {

bool COM_bnode_is_fast_node(const bNode &b_node)
{
  return !ELEM(b_node.type,
               CMP_NODE_BLUR,
               CMP_NODE_VECBLUR,
               CMP_NODE_BILATERALBLUR,
               CMP_NODE_DEFOCUS,
               CMP_NODE_BOKEHBLUR,
               CMP_NODE_GLARE,
               CMP_NODE_DBLUR,
               CMP_NODE_MOVIEDISTORTION,
               CMP_NODE_LENSDIST,
               CMP_NODE_DOUBLEEDGEMASK,
               CMP_NODE_DILATEERODE,
               CMP_NODE_DENOISE);
}

Node *COM_convert_bnode(bNode *b_node)
{
  Node *node = nullptr;

  /* ignore undefined nodes with missing or invalid node data */
  if (nodeTypeUndefined(b_node)) {
    return nullptr;
  }

  switch (b_node->type) {
    case CMP_NODE_VIEWER:
      node = new ViewerNode(b_node);
      break;
    case NODE_GROUP:
    case NODE_GROUP_INPUT:
    case NODE_GROUP_OUTPUT:
      /* handled in NodeCompiler */
      break;
    case CMP_NODE_IMAGE:
      node = new ImageNode(b_node);
      break;
    case CMP_NODE_GAMMA:
      node = new GammaNode(b_node);
      break;
    case CMP_NODE_DBLUR:
      node = new DirectionalBlurNode(b_node);
      break;
  }
  return node;
}

/* TODO(jbakker): make this an std::optional<NodeOperation>. */
NodeOperation *COM_convert_data_type(const NodeOperationOutput &from, const NodeOperationInput &to)
{
  const DataType src_data_type = from.getDataType();
  const DataType dst_data_type = to.getDataType();

  if (src_data_type == DataType::Value && dst_data_type == DataType::Color) {
    return new ConvertValueToColorOperation();
  }
  if (src_data_type == DataType::Value && dst_data_type == DataType::Vector) {
    return new ConvertValueToVectorOperation();
  }
  if (src_data_type == DataType::Color && dst_data_type == DataType::Value) {
    return new ConvertColorToValueOperation();
  }
  if (src_data_type == DataType::Color && dst_data_type == DataType::Vector) {
    return new ConvertColorToVectorOperation();
  }
  if (src_data_type == DataType::Vector && dst_data_type == DataType::Value) {
    return new ConvertVectorToValueOperation();
  }
  if (src_data_type == DataType::Vector && dst_data_type == DataType::Color) {
    return new ConvertVectorToColorOperation();
  }

  return nullptr;
}

void COM_convert_resolution(NodeOperationBuilder &builder,
                            NodeOperationOutput *fromSocket,
                            NodeOperationInput *toSocket)
{
  ResizeMode mode = toSocket->getResizeMode();

  NodeOperation *toOperation = &toSocket->getOperation();
  const float toWidth = toOperation->getWidth();
  const float toHeight = toOperation->getHeight();
  NodeOperation *fromOperation = &fromSocket->getOperation();
  const float fromWidth = fromOperation->getWidth();
  const float fromHeight = fromOperation->getHeight();
  bool doCenter = false;
  bool doScale = false;
  float addX = (toWidth - fromWidth) / 2.0f;
  float addY = (toHeight - fromHeight) / 2.0f;
  float scaleX = 0;
  float scaleY = 0;

  switch (mode) {
    case ResizeMode::None:
      break;
    case ResizeMode::Center:
      doCenter = true;
      break;
    case ResizeMode::FitWidth:
      doCenter = true;
      doScale = true;
      scaleX = scaleY = toWidth / fromWidth;
      break;
    case ResizeMode::FitHeight:
      doCenter = true;
      doScale = true;
      scaleX = scaleY = toHeight / fromHeight;
      break;
    case ResizeMode::FitAny:
      doCenter = true;
      doScale = true;
      scaleX = toWidth / fromWidth;
      scaleY = toHeight / fromHeight;
      if (scaleX < scaleY) {
        scaleX = scaleY;
      }
      else {
        scaleY = scaleX;
      }
      break;
    case ResizeMode::Stretch:
      doCenter = true;
      doScale = true;
      scaleX = toWidth / fromWidth;
      scaleY = toHeight / fromHeight;
      break;
  }

  if (doCenter) {
    NodeOperation *first = nullptr;
    ScaleOperation *scaleOperation = nullptr;
    if (doScale) {
      scaleOperation = new ScaleOperation();
      scaleOperation->getInputSocket(1)->setResizeMode(ResizeMode::None);
      scaleOperation->getInputSocket(2)->setResizeMode(ResizeMode::None);
      first = scaleOperation;
      SetValueOperation *sxop = new SetValueOperation();
      sxop->setValue(scaleX);
      builder.addLink(sxop->getOutputSocket(), scaleOperation->getInputSocket(1));
      SetValueOperation *syop = new SetValueOperation();
      syop->setValue(scaleY);
      builder.addLink(syop->getOutputSocket(), scaleOperation->getInputSocket(2));
      builder.addOperation(sxop);
      builder.addOperation(syop);

      unsigned int resolution[2] = {fromOperation->getWidth(), fromOperation->getHeight()};
      scaleOperation->setResolution(resolution);
      sxop->setResolution(resolution);
      syop->setResolution(resolution);
      builder.addOperation(scaleOperation);
    }

    TranslateOperation *translateOperation = new TranslateOperation();
    translateOperation->getInputSocket(1)->setResizeMode(ResizeMode::None);
    translateOperation->getInputSocket(2)->setResizeMode(ResizeMode::None);
    if (!first) {
      first = translateOperation;
    }
    SetValueOperation *xop = new SetValueOperation();
    xop->setValue(addX);
    builder.addLink(xop->getOutputSocket(), translateOperation->getInputSocket(1));
    SetValueOperation *yop = new SetValueOperation();
    yop->setValue(addY);
    builder.addLink(yop->getOutputSocket(), translateOperation->getInputSocket(2));
    builder.addOperation(xop);
    builder.addOperation(yop);

    unsigned int resolution[2] = {toOperation->getWidth(), toOperation->getHeight()};
    translateOperation->setResolution(resolution);
    xop->setResolution(resolution);
    yop->setResolution(resolution);
    builder.addOperation(translateOperation);

    if (doScale) {
      translateOperation->getInputSocket(0)->setResizeMode(ResizeMode::None);
      builder.addLink(scaleOperation->getOutputSocket(), translateOperation->getInputSocket(0));
    }

    /* remove previous link and replace */
    builder.removeInputLink(toSocket);
    first->getInputSocket(0)->setResizeMode(ResizeMode::None);
    toSocket->setResizeMode(ResizeMode::None);
    builder.addLink(fromSocket, first->getInputSocket(0));
    builder.addLink(translateOperation->getOutputSocket(), toSocket);
  }
}

}  // namespace blender::compositor
