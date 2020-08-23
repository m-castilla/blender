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

#include "COM_BlurBaseOperation.h"
#include "DNA_node_types.h"

class FastGaussianBlurOperation : public BlurBaseOperation {
 public:
  FastGaussianBlurOperation();
  static void IIR_gauss(PixelsImg &src_dst, float sigma, unsigned int channel, unsigned int xy);

 protected:
  virtual WriteType getWriteType() const
  {
    return WriteType::SINGLE_THREAD;
  }
  virtual bool canCompute() const override
  {
    return false;
  }
  virtual void execPixels(ExecutionManager &man) override;
};

enum {
  FAST_GAUSS_OVERLAY_MIN = -1,
  FAST_GAUSS_OVERLAY_NONE = 0,
  FAST_GAUSS_OVERLAY_MAX = 1,
};

class FastGaussianBlurValueOperation : public NodeOperation {
 private:
  float m_sigma;

  /**
   * -1: re-mix with darker
   *  0: do nothing
   *  1 re-mix with lighter */
  int m_overlay;

 public:
  FastGaussianBlurValueOperation();

  void setSigma(float sigma)
  {
    this->m_sigma = sigma;
  }

  /* used for DOF blurring ZBuffer */
  void setOverlay(int overlay)
  {
    this->m_overlay = overlay;
  }

 protected:
  virtual WriteType getWriteType() const
  {
    return WriteType::SINGLE_THREAD;
  }
  virtual void hashParams() override;
  virtual bool canCompute() const override
  {
    return false;
  }
  virtual void execPixels(ExecutionManager &man) override;
};
