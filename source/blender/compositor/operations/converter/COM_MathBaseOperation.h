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

#ifndef __COM_MATHBASEOPERATION_H__
#define __COM_MATHBASEOPERATION_H__
#include "COM_NodeOperation.h"

/**
 * this program converts an input color to an output value.
 * it assumes we are in sRGB color space.
 */
class MathBaseOperation : public NodeOperation {
 protected:
  /**
   * Prefetched reference to the inputProgram
   */
  NodeOperation *m_input1;
  NodeOperation *m_input2;
  NodeOperation *m_input3;

  bool m_useClamp;

 protected:
  /**
   * Default constructor
   */
  MathBaseOperation();

 public:
  /**
   * Initialize the execution
   */
  void initExecution();

  /**
   * Deinitialize the execution
   */
  void deinitExecution();

  /**
   * Determine resolution
   */
  // void determineResolution(int resolution[2], int preferredResolution[2]) override;

  void setUseClamp(bool value)
  {
    this->m_useClamp = value;
  }

 protected:
  virtual void hashParams() override;
};

class MathAddOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathSubtractOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathMultiplyOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathDivideOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathSineOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathCosineOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathTangentOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathHyperbolicSineOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathHyperbolicCosineOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathHyperbolicTangentOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathArcSineOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathArcCosineOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathArcTangentOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathPowerOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathLogarithmOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathMinimumOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathMaximumOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathRoundOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathLessThanOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
class MathGreaterThanOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathModuloOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathAbsoluteOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathRadiansOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathDegreesOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathArcTan2Operation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathFloorOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathCeilOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathFractOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathSqrtOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathInverseSqrtOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathSignOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathExponentOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathTruncOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathSnapOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathWrapOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathPingpongOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathCompareOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathMultiplyAddOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathSmoothMinOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class MathSmoothMaxOperation : public MathBaseOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
#endif
