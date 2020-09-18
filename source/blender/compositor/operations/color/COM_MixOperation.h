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

#ifndef __COM_MIXOPERATION_H__
#define __COM_MIXOPERATION_H__

#include "COM_NodeOperation.h"

/**
 * All this programs converts an input color to an output value.
 * it assumes we are in sRGB color space.
 */

class MixBaseOperation : public NodeOperation {
 protected:
  /**
   * Prefetched reference to the inputProgram
   */
  NodeOperation *m_input_value;
  NodeOperation *m_input_color1;
  NodeOperation *m_input_color2;
  bool m_valueAlphaMultiply;
  bool m_useClamp;

 public:
  /**
   * Default constructor
   */
  MixBaseOperation();

  /**
   * Initialize the execution
   */
  void initExecution() override;

  /**
   * Deinitialize the execution
   */
  void deinitExecution() override;

  // void determineResolution(int resolution[2], int preferredResolution[2]);

  void setUseValueAlphaMultiply(const bool value)
  {
    this->m_valueAlphaMultiply = value;
  }
  inline bool useValueAlphaMultiply()
  {
    return this->m_valueAlphaMultiply;
  }
  void setUseClamp(bool value)
  {
    this->m_useClamp = value;
  }

 protected:
  virtual void hashParams() override;
  virtual void execPixels(ExecutionManager &man) override;
};

class MixAddOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixBlendOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixColorBurnOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixColorOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixDarkenOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixDifferenceOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixDivideOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixDodgeOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixGlareOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixHueOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixLightenOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixLinearLightOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixMultiplyOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixOverlayOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixSaturationOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixScreenOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixSoftLightOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixSubtractOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MixValueOperation : public MixBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

#endif
