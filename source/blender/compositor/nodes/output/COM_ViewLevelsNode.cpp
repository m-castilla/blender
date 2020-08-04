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

#include "COM_ViewLevelsNode.h"
#include "COM_CalculateMeanOperation.h"
#include "COM_CalculateStandardDeviationOperation.h"
#include "COM_ExecutionSystem.h"
#include "COM_SetValueOperation.h"

ViewLevelsNode::ViewLevelsNode(bNode *editorNode) : Node(editorNode)
{
  /* pass */
}

void ViewLevelsNode::convertToOperations(NodeConverter &converter,
                                         const CompositorContext & /*context*/) const
{
  NodeInput *input = this->getInputSocket(0);
  if (input->isLinked()) {
    // add preview to inputSocket;

    /* calculate mean operation */
    {
      CalculateMeanOperation *mean_op = new CalculateMeanOperation();
      mean_op->setSetting(this->getbNode()->custom1);

      converter.addOperation(mean_op);
      converter.mapInputSocket(input, mean_op->getInputSocket(0));
      converter.mapOutputSocket(this->getOutputSocket(0), mean_op->getOutputSocket());

      CalculateStandardDeviationOperation *std_op = new CalculateStandardDeviationOperation();
      std_op->setSetting(this->getbNode()->custom1);
      converter.addOperation(std_op);

      converter.mapInputSocket(input, std_op->getInputSocket(0));
      converter.addLink(mean_op->getOutputSocket(), std_op->getInputSocket(1));

      converter.mapOutputSocket(this->getOutputSocket(1), std_op->getOutputSocket());
    }

    /* calculate standard deviation operation */
    //{
    //  CalculateMeanOperation *mean_op = new CalculateMeanOperation();
    //  mean_op->setSetting(this->getbNode()->custom1);
    //  converter.addOperation(mean_op);
    //  converter.mapInputSocket(input, mean_op->getInputSocket(0));

    //  CalculateStandardDeviationOperation *std_op = new CalculateStandardDeviationOperation();
    //  std_op->setSetting(this->getbNode()->custom1);
    //  converter.addOperation(std_op);

    //  converter.addLink(mean_op->getOutputSocket(), std_op->getInputSocket(0));

    //  converter.mapOutputSocket(this->getOutputSocket(1), std_op->getOutputSocket());
    //}
  }
  else {
    converter.addOutputValue(getOutputSocket(0), 0.0f);
    converter.addOutputValue(getOutputSocket(1), 0.0f);
  }
}
