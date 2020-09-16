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

#pragma once

#include <string.h>

#include "COM_PlaneDistortCommonOperation.h"

#include "DNA_movieclip_types.h"
#include "DNA_tracking_types.h"

#include "BLI_listbase.h"
#include "BLI_string.h"

class PlaneCornerPinMaskOperation : public PlaneDistortMaskOperation {
 private:
  bool m_corners_ready;
  int corners_input_offset;

 public:
  PlaneCornerPinMaskOperation();

  void readCorners(NodeOperation *reader_op, ExecutionManager &man) override;

  ResolutionType determineResolution(int resolution[2],
                                     int preferredResolution[2],
                                     bool setResolution) override;
};

class PlaneCornerPinWarpImageOperation : public PlaneDistortWarpImageOperation {
 private:
  bool m_corners_ready;
  int corners_input_offset;

 public:
  PlaneCornerPinWarpImageOperation();

  void readCorners(NodeOperation *reader_op, ExecutionManager &man) override;
};
