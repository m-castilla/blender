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

#include <stddef.h>

#include "BKE_tracking.h"
#include "DNA_movieclip_types.h"

#include "COM_ExecutionSystem.h"
#include "COM_GlobalManager.h"
#include "COM_MovieClipAttributeOperation.h"
#include "COM_Stabilize2dNode.h"
#include "COM_TransformOperation.h"
#include "COM_UiConvert.h"

Stabilize2dNode::Stabilize2dNode(bNode *editorNode) : Node(editorNode)
{
  /* pass */
}

void Stabilize2dNode::convertToOperations(NodeConverter &converter,
                                          CompositorContext &context) const
{
  auto sampler = GlobalMan->getContext()->getDefaultSampler();
  sampler.filter = UiConvert::pixelInterpolation(this->getbNode()->custom1);

  bNode *editorNode = this->getbNode();
  NodeInput *imageInput = this->getInputSocket(0);
  MovieClip *clip = (MovieClip *)editorNode->id;
  bool invert = (editorNode->custom2 & CMP_NODEFLAG_STABILIZE_INVERSE) != 0;

  TransformOperation *transformOp = new TransformOperation();
  transformOp->setSampler(sampler);

  MovieClipAttributeOperation *scaleAttribute = new MovieClipAttributeOperation();
  MovieClipAttributeOperation *angleAttribute = new MovieClipAttributeOperation();
  MovieClipAttributeOperation *xAttribute = new MovieClipAttributeOperation();
  MovieClipAttributeOperation *yAttribute = new MovieClipAttributeOperation();

  scaleAttribute->setAttribute(MovieClipAttribute::MCA_SCALE);
  scaleAttribute->setFramenumber(context.getCurrentFrame());
  scaleAttribute->setMovieClip(clip);
  scaleAttribute->setInvert(invert);

  angleAttribute->setAttribute(MovieClipAttribute::MCA_ANGLE);
  angleAttribute->setFramenumber(context.getCurrentFrame());
  angleAttribute->setMovieClip(clip);
  angleAttribute->setInvert(invert);

  xAttribute->setAttribute(MovieClipAttribute::MCA_X);
  xAttribute->setFramenumber(context.getCurrentFrame());
  xAttribute->setMovieClip(clip);
  xAttribute->setInvert(invert);

  yAttribute->setAttribute(MovieClipAttribute::MCA_Y);
  yAttribute->setFramenumber(context.getCurrentFrame());
  yAttribute->setMovieClip(clip);
  yAttribute->setInvert(invert);

  converter.addOperation(scaleAttribute);
  converter.addOperation(angleAttribute);
  converter.addOperation(xAttribute);
  converter.addOperation(yAttribute);
  converter.addOperation(transformOp);

  converter.mapInputSocket(imageInput, transformOp->getInputSocket(0), false);
  converter.addLink(xAttribute->getOutputSocket(), transformOp->getInputSocket(1));
  converter.addLink(yAttribute->getOutputSocket(), transformOp->getInputSocket(2));
  converter.addLink(angleAttribute->getOutputSocket(), transformOp->getInputSocket(3));
  converter.addLink(scaleAttribute->getOutputSocket(), transformOp->getInputSocket(4));
  converter.mapOutputSocket(getOutputSocket(), transformOp->getOutputSocket(), false);
}
