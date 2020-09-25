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

#include "BLI_listbase.h"
#include "BLI_utildefines.h"
#include "COM_NodeOperation.h"
#include "DNA_scene_types.h"
#include "MEM_guardedalloc.h"

#include "RE_pipeline.h"

/**
 * Base class for all renderlayeroperations
 */
class VideoSequencerOperation : public NodeOperation {
 private:
  /**
   * Reference to the scene object.
   */
  ImBuf *m_seq_frame;
  int m_n_frame;
  int m_n_channel;
  Scene *m_scene;
  bool m_is_rendering;

  /**
   * Determine the output resolution. The resolution is retrieved from the Renderer
   */
  ResolutionType determineResolution(int resolution[2],
                                     int preferredResolution[2],
                                     bool setResolution) override;

 public:
  VideoSequencerOperation();

  void setNChannel(int n_channel)
  {
    m_n_channel = n_channel;
  }

  void deinitExecution() override;

 protected:
  void execPixels(ExecutionManager &man) override;
  bool canCompute() const override
  {
    return false;
  }
  void hashParams() override;

 private:
  void assureSequencerRender();
};
