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

#include "COM_SimpleOperation.h"

namespace blender::compositor {

class ConvertBaseOperation : public SimpleOperation {
 public:
  void execPixelsMultiCPU(const rcti &render_rect,
                          CPUBuffer<float> &output,
                          blender::Span<const CPUBuffer<float> *> inputs,
                          ExecutionSystem *exec_system,
                          int current_pass) override;

  virtual void execPixelsRowCPU(float *output,
                                const float *output_end,
                                int output_jump,
                                const float *input,
                                int input_jump) = 0;
};

class ConvertValueToColorOperation : public ConvertBaseOperation {
 public:
  ConvertValueToColorOperation();

  void execPixelsRowCPU(float *output,
                        const float *output_end,
                        int output_jump,
                        const float *input,
                        int input_jump) override;
};

class ConvertColorToValueOperation : public ConvertBaseOperation {
 public:
  ConvertColorToValueOperation();

  void execPixelsRowCPU(float *output,
                        const float *output_end,
                        int output_jump,
                        const float *input,
                        int input_jump) override;
};

class ConvertColorToVectorOperation : public ConvertBaseOperation {
 public:
  ConvertColorToVectorOperation();

  void execPixelsRowCPU(float *output,
                        const float *output_end,
                        int output_jump,
                        const float *input,
                        int input_jump) override;
};

class ConvertValueToVectorOperation : public ConvertBaseOperation {
 public:
  ConvertValueToVectorOperation();

  void execPixelsRowCPU(float *output,
                        const float *output_end,
                        int output_jump,
                        const float *input,
                        int input_jump) override;
};

class ConvertVectorToColorOperation : public ConvertBaseOperation {
 public:
  ConvertVectorToColorOperation();

  void execPixelsRowCPU(float *output,
                        const float *output_end,
                        int output_jump,
                        const float *input,
                        int input_jump) override;
};

class ConvertVectorToValueOperation : public ConvertBaseOperation {
 public:
  ConvertVectorToValueOperation();

  void execPixelsRowCPU(float *output,
                        const float *output_end,
                        int output_jump,
                        const float *input,
                        int input_jump) override;
};

class ConvertPremulToStraightOperation : public ConvertBaseOperation {
 public:
  ConvertPremulToStraightOperation();

  void execPixelsRowCPU(float *output,
                        const float *output_end,
                        int output_jump,
                        const float *input,
                        int input_jump) override;
};

class ConvertStraightToPremulOperation : public ConvertBaseOperation {
 public:
  ConvertStraightToPremulOperation();

  void execPixelsRowCPU(float *output,
                        const float *output_end,
                        int output_jump,
                        const float *input,
                        int input_jump) override;
};

}  // namespace blender::compositor
