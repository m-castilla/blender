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

#include "COM_Buffer.h"
#include "COM_NodeOperation.h"
#include "COM_QualityStepHelper.h"

#define MAX_GAUSSTAB_RADIUS 30000

class BlurBaseOperation : public NodeOperation, public QualityStepHelper {
 protected:
  BlurBaseOperation(SocketType socket_type);
  int get_gausstab_n_elems(int size)
  {
    return 2 * size + 1;
  }
  int make_gausstab(float *buffer, int buffer_start, float rad, int size);

  // returned buffer must be recycled
  TmpBuffer *make_gausstab(float rad, int size);
  // returned buffer must be recycled
  TmpBuffer *make_dist_fac_inverse(float rad, int size, int falloff);

  // void updateSize();

  NodeBlurData m_data;

  float m_size;

  bool m_extend_bounds;

 public:
  void initExecution();
  void setData(const NodeBlurData *data);
  void setSize(float size)
  {
    this->m_size = size;
  }
  void setExtendBounds(bool extend_bounds)
  {
    this->m_extend_bounds = extend_bounds;
  }

  ResolutionType determineResolution(int resolution[2],
                                     int preferredResolution[2],
                                     bool setResolution) override;

 protected:
  virtual void hashParams();
};
