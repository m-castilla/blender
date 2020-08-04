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

#ifndef __COM_SCALEOPERATION_H__
#define __COM_SCALEOPERATION_H__

#include "COM_NodeOperation.h"

class ScaleOperation : public NodeOperation {
 private:
  NodeOperation *m_inputOperation;
  NodeOperation *m_inputXOperation;
  NodeOperation *m_inputYOperation;
  bool m_relative;
  PixelsSampler m_sampler;

 public:
  ScaleOperation();
  void hashParams() override;
  void initExecution() override;
  void deinitExecution() override;
  void setRelative(bool relative)
  {
    m_relative = relative;
  }

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class ScaleFixedSizeOperation : public NodeOperation {
  PixelsSampler m_sampler;
  NodeOperation *m_inputOperation;
  int m_newWidth;
  int m_newHeight;
  float m_relX;
  float m_relY;

  /* center is only used for aspect correction */
  float m_offsetX;
  float m_offsetY;
  bool m_is_aspect;
  bool m_is_crop;
  /* set from other properties on initialization,
   * check if we need to apply offset */
  bool m_is_offset;

 public:
  ScaleFixedSizeOperation();

  void determineResolution(int resolution[2],
                           int preferredResolution[2],
                           bool setResolution) override;

  void initExecution() override;
  void deinitExecution() override;
  void setNewWidth(int width)
  {
    this->m_newWidth = width;
  }
  void setNewHeight(int height)
  {
    this->m_newHeight = height;
  }
  void setIsAspect(bool is_aspect)
  {
    this->m_is_aspect = is_aspect;
  }
  void setIsCrop(bool is_crop)
  {
    this->m_is_crop = is_crop;
  }
  void setOffset(float x, float y)
  {
    this->m_offsetX = x;
    this->m_offsetY = y;
  }

 protected:
  virtual void execPixels(ExecutionManager &man) override;
  virtual void hashParams() override;
};

#endif
