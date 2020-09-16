/* This program is free software; you can redistribute it and/or
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
 * Copyright 2014, Blender Foundation.
 */

#include "MEM_guardedalloc.h"

#include "BLI_listbase.h"
#include "BLI_math.h"
#include "BLI_math_color.h"

#include "BKE_node.h"

#include "COM_ExecutionManager.h"
#include "COM_PlaneCornerPinOperation.h"

static bool check_corners(float corners[4][2])
{
  int i, next, prev;
  float cross = 0.0f;

  for (i = 0; i < 4; i++) {
    float v1[2], v2[2], cur_cross;

    next = (i + 1) % 4;
    prev = (4 + i - 1) % 4;

    sub_v2_v2v2(v1, corners[i], corners[prev]);
    sub_v2_v2v2(v2, corners[next], corners[i]);

    cur_cross = cross_v2v2(v1, v2);
    if (fabsf(cur_cross) <= FLT_EPSILON) {
      return false;
    }

    if (cross == 0.0f) {
      cross = cur_cross;
    }
    else if (cross * cur_cross < 0.0f) {
      return false;
    }
  }

  return true;
}

static void readCornersFromInputs(float *inputs[4], float corners[4][2])
{
  for (int i = 0; i < 4; i++) {
    corners[i][0] = inputs[i] ? inputs[i][0] : 0.0f;
    corners[i][1] = inputs[i] ? inputs[i][1] : 0.0f;
  }

  /* convexity check:
   * concave corners need to be prevented, otherwise
   * BKE_tracking_homography_between_two_quads will freeze
   */
  if (!check_corners(corners)) {
    /* simply revert to default corners
     * there could be a more elegant solution,
     * this prevents freezing at least.
     */
    corners[0][0] = 0.0f;
    corners[0][1] = 0.0f;
    corners[1][0] = 1.0f;
    corners[1][1] = 0.0f;
    corners[2][0] = 1.0f;
    corners[2][1] = 1.0f;
    corners[3][0] = 0.0f;
    corners[3][1] = 1.0f;
  }
}

/* ******** PlaneCornerPinMaskOperation ******** */

PlaneCornerPinMaskOperation::PlaneCornerPinMaskOperation()
    : PlaneDistortMaskOperation(), m_corners_ready(false)
{
  corners_input_offset = getNumberOfInputSockets();
  addInputSocket(SocketType::VECTOR);
  addInputSocket(SocketType::VECTOR);
  addInputSocket(SocketType::VECTOR);
  addInputSocket(SocketType::VECTOR);
}

void PlaneCornerPinMaskOperation::readCorners(NodeOperation *reader_op, ExecutionManager &man)
{
  /* get corner values once, by reading inputs at (0,0)
   * XXX this assumes invariable values (no image inputs),
   * we don't have a nice generic system for that yet
   */
  if (!m_corners_ready) {
    float *inputs[4];  // 4 corners inputs
    for (int i = 0; i < 4; i++) {
      inputs[i] =
          getInputOperation(corners_input_offset + i)->getSinglePixel(reader_op, man, 0, 0);
    }
    float corners[4][2] = {0};
    if (man.canExecPixels()) {
      readCornersFromInputs(inputs, corners);
      saveCorners(corners, true, 0);
      m_corners_ready = true;
    }
  }
}

ResolutionType PlaneCornerPinMaskOperation::determineResolution(int resolution[2],
                                                                int preferredResolution[2],
                                                                bool /*setResolution*/)
{
  resolution[0] = preferredResolution[0];
  resolution[1] = preferredResolution[1];
  return ResolutionType::Determined;
}

/* ******** PlaneCornerPinWarpImageOperation ******** */

PlaneCornerPinWarpImageOperation::PlaneCornerPinWarpImageOperation()
    : PlaneDistortWarpImageOperation(), m_corners_ready(false)
{
  corners_input_offset = getNumberOfInputSockets();
  addInputSocket(SocketType::VECTOR);
  addInputSocket(SocketType::VECTOR);
  addInputSocket(SocketType::VECTOR);
  addInputSocket(SocketType::VECTOR);
}

void PlaneCornerPinWarpImageOperation::readCorners(NodeOperation *reader_op, ExecutionManager &man)
{
  /* get corner values once, by reading inputs at (0,0)
   * XXX this assumes invariable values (no image inputs),
   * we don't have a nice generic system for that yet
   */
  if (!m_corners_ready) {
    float *inputs[4];  // 4 corners inputs
    for (int i = 0; i < 4; i++) {
      inputs[i] =
          getInputOperation(corners_input_offset + i)->getSinglePixel(reader_op, man, 0, 0);
    }
    float corners[4][2] = {0};
    if (man.canExecPixels()) {
      readCornersFromInputs(inputs, corners);
      saveCorners(corners, true, 0);
      m_corners_ready = true;
    }
  }
}