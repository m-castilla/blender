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
#include "COM_kernel_cpu.h"

#define COM_CONVOLUTION_FILTER_SIZE 9
class ConvolutionFilterOperation : public NodeOperation {
 private:
  int m_filterWidth;
  int m_filterHeight;

 protected:
  float m_filter[COM_CONVOLUTION_FILTER_SIZE];
  CCL::float4 m_filter_f4[COM_CONVOLUTION_FILTER_SIZE];
  CCL::float3 m_filter_f3[COM_CONVOLUTION_FILTER_SIZE];

 public:
  ConvolutionFilterOperation();
  void set3x3Filter(
      float f1, float f2, float f3, float f4, float f5, float f6, float f7, float f8, float f9);

 protected:
  virtual void hashParams() override;
  virtual void execPixels(ExecutionManager &man) override;
};
