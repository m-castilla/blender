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
 * Copyright 2012, Blender Foundation.
 */

#pragma once

#include "BLI_listbase.h"
#include "COM_NodeOperation.h"
#include "DNA_mask_types.h"
#include "DNA_node_types.h"
#include "IMB_imbuf_types.h"
#include <algorithm>

/**
 * Class with implementation of mask rasterization
 */
class MaskOperation : public NodeOperation {
 protected:
  Mask *m_mask;

  /* note, these are used more like aspect,
   * but they _do_ impact on mask detail */
  int m_maskWidth;
  int m_maskHeight;
  float m_maskWidthInv;  /* 1 / m_maskWidth  */
  float m_maskHeightInv; /* 1 / m_maskHeight */
  float m_mask_px_ofs[2];

  float m_frame_shutter;
  int m_frame_number;

  bool m_do_feather;

  struct MaskRasterHandle *m_rasterMaskHandles[CMP_NODE_MASK_MBLUR_SAMPLES_MAX];
  unsigned int m_rasterMaskHandleTot;

  ResolutionType determineResolution(int resolution[2],
                                     int preferredResolution[2],
                                     bool setResolution) override;

 public:
  MaskOperation();

  void initExecution();
  void deinitExecution();

  void setMask(Mask *mask)
  {
    this->m_mask = mask;
  }
  void setMaskWidth(int width)
  {
    this->m_maskWidth = width;
    this->m_maskWidthInv = 1.0f / (float)width;
    this->m_mask_px_ofs[0] = this->m_maskWidthInv * 0.5f;
  }
  void setMaskHeight(int height)
  {
    this->m_maskHeight = height;
    this->m_maskHeightInv = 1.0f / (float)height;
    this->m_mask_px_ofs[1] = this->m_maskHeightInv * 0.5f;
  }
  void setFramenumber(int frame_number)
  {
    this->m_frame_number = frame_number;
  }
  void setFeather(bool feather)
  {
    this->m_do_feather = feather;
  }

  void setMotionBlurSamples(int samples)
  {
    this->m_rasterMaskHandleTot = std::min(std::max(1, samples), CMP_NODE_MASK_MBLUR_SAMPLES_MAX);
  }
  void setMotionBlurShutter(float shutter)
  {
    this->m_frame_shutter = shutter;
  }

 protected:
  void execPixels(ExecutionManager &man) override;
  bool canCompute() const override
  {
    return false;
  }
  void hashParams() override;
};
