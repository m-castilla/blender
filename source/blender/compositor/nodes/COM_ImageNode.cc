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

#include "COM_ImageNode.h"
#include "BKE_node.h"
#include "BLI_utildefines.h"
#include "COM_ConvertOperation.h"
#include "COM_ExecutionSystem.h"
#include "COM_ImageOperation.h"
#include "COM_SetColorOperation.h"
#include "COM_SetValueOperation.h"
#include "COM_SetVectorOperation.h"

namespace blender::compositor {

ImageNode::ImageNode(bNode *editorNode) : Node(editorNode)
{
  /* pass */
}
void ImageNode::convertToOperations(NodeConverter &converter,
                                    const CompositorContext &context) const
{
  /** Image output */
  NodeOutput *outputImage = this->getOutputSocket(0);
  bNode *editorNode = this->getbNode();
  Image *image = (Image *)editorNode->id;
  ImageUser *imageuser = (ImageUser *)editorNode->storage;
  int framenumber = context.getFramenumber();
  bool outputStraightAlpha = (editorNode->custom1 & CMP_NODE_IMAGE_USE_STRAIGHT_OUTPUT) != 0;
  BKE_image_user_frame_calc(image, imageuser, context.getFramenumber());
  /* force a load, we assume iuser index will be set OK anyway */
  if (image && image->type == IMA_TYPE_MULTILAYER) {
    BLI_assert(!"IMA_TYPE_MULTILAYER is not supported");
  }
  else {
    const int64_t numberOfOutputs = getOutputSockets().size();
    if (numberOfOutputs > 0) {
      ImageOperation *operation = new ImageOperation();
      operation->setImage(image);
      operation->setImageUser(imageuser);
      operation->setFramenumber(framenumber);
      operation->setRenderData(context.getRenderData());
      operation->setViewName(context.getViewName());
      converter.addOperation(operation);

      if (outputStraightAlpha) {
        NodeOperation *alphaConvertOperation = new ConvertPremulToStraightOperation();

        converter.addOperation(alphaConvertOperation);
        converter.mapOutputSocket(outputImage, alphaConvertOperation->getOutputSocket());
        converter.addLink(operation->getOutputSocket(0), alphaConvertOperation->getInputSocket(0));
      }
      else {
        converter.mapOutputSocket(outputImage, operation->getOutputSocket());
      }

      converter.addPreview(operation->getOutputSocket());
    }

    if (numberOfOutputs > 1) {
      NodeOutput *alphaImage = this->getOutputSocket(1);
      ImageAlphaOperation *alphaOperation = new ImageAlphaOperation();
      alphaOperation->setImage(image);
      alphaOperation->setImageUser(imageuser);
      alphaOperation->setFramenumber(framenumber);
      alphaOperation->setRenderData(context.getRenderData());
      alphaOperation->setViewName(context.getViewName());
      converter.addOperation(alphaOperation);

      converter.mapOutputSocket(alphaImage, alphaOperation->getOutputSocket());
    }
    if (numberOfOutputs > 2) {
      NodeOutput *depthImage = this->getOutputSocket(2);
      ImageDepthOperation *depthOperation = new ImageDepthOperation();
      depthOperation->setImage(image);
      depthOperation->setImageUser(imageuser);
      depthOperation->setFramenumber(framenumber);
      depthOperation->setRenderData(context.getRenderData());
      depthOperation->setViewName(context.getViewName());
      converter.addOperation(depthOperation);

      converter.mapOutputSocket(depthImage, depthOperation->getOutputSocket());
    }
    if (numberOfOutputs > 3) {
      /* happens when unlinking image datablock from multilayer node */
      for (int i = 3; i < numberOfOutputs; i++) {
        NodeOutput *output = this->getOutputSocket(i);
        NodeOperation *operation = nullptr;
        switch (output->getDataType()) {
          case DataType::Value: {
            SetValueOperation *valueoperation = new SetValueOperation();
            valueoperation->setValue(0.0f);
            operation = valueoperation;
            break;
          }
          case DataType::Vector: {
            SetVectorOperation *vectoroperation = new SetVectorOperation();
            vectoroperation->setX(0.0f);
            vectoroperation->setY(0.0f);
            vectoroperation->setW(0.0f);
            operation = vectoroperation;
            break;
          }
          case DataType::Color: {
            SetColorOperation *coloroperation = new SetColorOperation();
            coloroperation->setChannel1(0.0f);
            coloroperation->setChannel2(0.0f);
            coloroperation->setChannel3(0.0f);
            coloroperation->setChannel4(0.0f);
            operation = coloroperation;
            break;
          }
        }

        if (operation) {
          /* not supporting multiview for this generic case */
          converter.addOperation(operation);
          converter.mapOutputSocket(output, operation->getOutputSocket());
        }
      }
    }
  }
}  // namespace blender::compositor

}  // namespace blender::compositor
