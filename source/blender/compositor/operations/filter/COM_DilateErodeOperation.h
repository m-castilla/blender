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

#ifndef __COM_DILATEERODEOPERATION_H__
#define __COM_DILATEERODEOPERATION_H__
#include "COM_NodeOperation.h"

class DilateErodeThresholdOperation : public NodeOperation {
 private:
  float m_distance;
  float m__switch;
  float m_inset;

  /**
   * determines the area of interest to track pixels
   * keep this one as small as possible for speed gain.
   */
  int m_scope;

 public:
  DilateErodeThresholdOperation();

  void initExecution();

  void setDistance(float distance)
  {
    this->m_distance = distance;
  }
  void setSwitch(float sw)
  {
    this->m__switch = sw;
  }
  void setInset(float inset)
  {
    this->m_inset = inset;
  }

 protected:
  virtual void hashParams() override;
  virtual void execPixels(ExecutionManager &man) override;
};

class DilateDistanceOperation : public NodeOperation {
 protected:
  float m_distance;
  int m_scope;

 public:
  DilateDistanceOperation();

  void initExecution();

  void setDistance(float distance)
  {
    this->m_distance = distance;
  }

 protected:
  virtual void hashParams() override;
  virtual void execPixels(ExecutionManager &man) override;
};
class ErodeDistanceOperation : public DilateDistanceOperation {
 public:
  ErodeDistanceOperation();

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class DilateStepOperation : public NodeOperation {
 protected:
  int m_iterations;

 public:
  DilateStepOperation();

  void setIterations(int iterations)
  {
    this->m_iterations = iterations;
  }

 protected:
  virtual void hashParams() override;
  virtual bool canCompute() const override
  {
    return false;
  }
  virtual void execPixels(ExecutionManager &man) override;
};

class ErodeStepOperation : public DilateStepOperation {
 public:
  ErodeStepOperation();

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

#endif
