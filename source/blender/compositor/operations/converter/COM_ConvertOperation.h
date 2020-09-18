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

#include "COM_NodeOperation.h"

class ConvertBaseOperation : public NodeOperation {
 protected:
  NodeOperation *m_inputOperation;

 public:
  ConvertBaseOperation();

  void initExecution() override;
  void deinitExecution() override;
};

class ConvertValueToColorOperation : public ConvertBaseOperation {
 public:
  ConvertValueToColorOperation();

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class ConvertColorToValueOperation : public ConvertBaseOperation {
 public:
  ConvertColorToValueOperation();

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class ConvertColorToBWOperation : public ConvertBaseOperation {
 public:
  ConvertColorToBWOperation();

 protected:
  void execPixels(ExecutionManager &man) override;
  bool canCompute() const override
  {
    return false;
  }
};

class ConvertColorToVectorOperation : public ConvertBaseOperation {
 public:
  ConvertColorToVectorOperation();

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class ConvertValueToVectorOperation : public ConvertBaseOperation {
 public:
  ConvertValueToVectorOperation();

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class ConvertVectorToColorOperation : public ConvertBaseOperation {
 public:
  ConvertVectorToColorOperation();

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class ConvertVectorToValueOperation : public ConvertBaseOperation {
 public:
  ConvertVectorToValueOperation();

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class ConvertRGBToYCCOperation : public ConvertBaseOperation {
 private:
  /** YCbCr mode (Jpeg, ITU601, ITU709) */
  int m_mode;

 public:
  ConvertRGBToYCCOperation();
  /** Set the YCC mode */
  void setMode(int mode);

 protected:
  virtual void execPixels(ExecutionManager &man) override;
  void hashParams();
};

class ConvertYCCToRGBOperation : public ConvertBaseOperation {
 private:
  /** YCbCr mode (Jpeg, ITU601, ITU709) */
  int m_mode;

 public:
  ConvertYCCToRGBOperation();
  /** Set the YCC mode */
  void setMode(int mode);

 protected:
  virtual void execPixels(ExecutionManager &man) override;
  void hashParams();
};

class ConvertRGBToYUVOperation : public ConvertBaseOperation {
 public:
  ConvertRGBToYUVOperation();

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class ConvertYUVToRGBOperation : public ConvertBaseOperation {
 public:
  ConvertYUVToRGBOperation();

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class ConvertRGBToHSVOperation : public ConvertBaseOperation {
 public:
  ConvertRGBToHSVOperation();

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class ConvertHSVToRGBOperation : public ConvertBaseOperation {
 public:
  ConvertHSVToRGBOperation();

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class ConvertPremulToStraightOperation : public ConvertBaseOperation {
 public:
  ConvertPremulToStraightOperation();

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class ConvertStraightToPremulOperation : public ConvertBaseOperation {
 public:
  ConvertStraightToPremulOperation();

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

class SeparateChannelOperation : public NodeOperation {
 private:
  NodeOperation *m_inputOperation;
  int m_channel;

 public:
  SeparateChannelOperation();

  void initExecution();
  void deinitExecution();

  void setChannel(int channel)
  {
    this->m_channel = channel;
  }

 protected:
  void hashParams();
  virtual void execPixels(ExecutionManager &man) override;
};

class CombineChannelsOperation : public NodeOperation {
 private:
  NodeOperation *m_inputChannel0Operation;
  NodeOperation *m_inputChannel1Operation;
  NodeOperation *m_inputChannel2Operation;
  NodeOperation *m_inputChannel3Operation;

 public:
  CombineChannelsOperation();

  void initExecution();
  void deinitExecution();

 protected:
  virtual void execPixels(ExecutionManager &man) override;
};
