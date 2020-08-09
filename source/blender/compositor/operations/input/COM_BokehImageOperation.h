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

#pragma once

#include "COM_NodeOperation.h"

/**
 * \brief The BokehImageOperation class is an operation that creates an image useful to mimic the
 *internals of a camera.
 *
 * features:
 *  - number of flaps
 *  - angle offset of the flaps
 *  - rounding of the flaps (also used to make a circular lens)
 *  - simulate catadioptric
 *  - simulate lens-shift
 *
 * Per pixel the algorithm determines the edge of the bokeh on the same line as the center of the
 *image and the pixel is evaluating.
 *
 * The edge is detected by finding the closest point on the direct line between the two nearest
 *flap-corners. this edge is interpolated with a full circle. Result of this edge detection is
 *stored as the distance between the center of the image and the edge.
 *
 * catadioptric lenses are simulated to interpolate between the center of the image and the
 *distance of the edge. We now have three distances:
 *  - distance between the center of the image and the pixel to be evaluated
 *  - distance between the center of the image and the outer-edge
 *  - distance between the center of the image and the inner-edge
 *
 * With a simple compare it can be detected if the evaluated pixel is between the outer and inner
 *edge.
 */
struct NodeBokehImage;
class BokehImageOperation : public NodeOperation {
 private:
  /**
   * \brief Settings of the bokeh image
   */
  NodeBokehImage *m_data;

  /**
   * \brief precalced center of the image
   */
  float m_center[2];

  /**
   * \brief distance of a full circle lens
   */
  float m_circularDistance;

  /**
   * \brief radius when the first flap starts
   */
  float m_flapRad;

  /**
   * \brief radians of a single flap
   */
  float m_flapRadAdd;

  /**
   * \brief should the m_data field by deleted when this operation is finished
   */
  bool m_deleteData;

 public:
  BokehImageOperation();

  /**
   * \brief Initialize the execution
   */
  void initExecution();

  /**
   * \brief Deinitialize the execution
   */
  void deinitExecution();

  /**
   * \brief determine the resolution of this operation. currently fixed at [COM_BLUR_BOKEH_PIXELS,
   * COM_BLUR_BOKEH_PIXELS] \param resolution: \param preferredResolution:
   */
  void determineResolution(int resolution[2],
                           int preferredResolution[2],
                           DetermineResolutionMode mode,
                           bool setResolution) override;

  /**
   * \brief set the node data
   * \param data:
   */
  void setData(NodeBokehImage *data)
  {
    this->m_data = data;
  }

  /**
   * \brief deleteDataOnFinish
   *
   * There are cases that the compositor uses this operation on its own (see defocus node)
   * the deleteDataOnFinish must only be called when the data has been created by the compositor.
   *It should not be called when the data has been created by the node-editor/user.
   */
  void deleteDataOnFinish()
  {
    this->m_deleteData = true;
  }

 protected:
  void hashParams() override;
  void execPixels(ExecutionManager &man) override;
};
