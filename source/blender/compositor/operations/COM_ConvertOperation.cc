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

#include "COM_ConvertOperation.h"

#include "IMB_colormanagement.h"

namespace blender::compositor {

ConvertBaseOperation::ConvertBaseOperation()
{
  this->m_inputOperation = nullptr;
}

void ConvertBaseOperation::initExecution()
{
  this->m_inputOperation = this->getInputSocketReader(0);
}

void ConvertBaseOperation::deinitExecution()
{
  this->m_inputOperation = nullptr;
}

/* ******** Value to Color ******** */

ConvertValueToColorOperation::ConvertValueToColorOperation() : ConvertBaseOperation()
{
  this->addInputSocket(DataType::Value);
  this->addOutputSocket(DataType::Color);
}

void ConvertValueToColorOperation::executePixelSampled(float output[4],
                                                       float x,
                                                       float y,
                                                       PixelSampler sampler)
{
  float value;
  this->m_inputOperation->readSampled(&value, x, y, sampler);
  output[0] = output[1] = output[2] = value;
  output[3] = 1.0f;
}

/* ******** Color to Value ******** */

ConvertColorToValueOperation::ConvertColorToValueOperation() : ConvertBaseOperation()
{
  this->addInputSocket(DataType::Color);
  this->addOutputSocket(DataType::Value);
}

void ConvertColorToValueOperation::executePixelSampled(float output[4],
                                                       float x,
                                                       float y,
                                                       PixelSampler sampler)
{
  float inputColor[4];
  this->m_inputOperation->readSampled(inputColor, x, y, sampler);
  output[0] = (inputColor[0] + inputColor[1] + inputColor[2]) / 3.0f;
}

/* ******** Color to Vector ******** */

ConvertColorToVectorOperation::ConvertColorToVectorOperation() : ConvertBaseOperation()
{
  this->addInputSocket(DataType::Color);
  this->addOutputSocket(DataType::Vector);
}

void ConvertColorToVectorOperation::executePixelSampled(float output[4],
                                                        float x,
                                                        float y,
                                                        PixelSampler sampler)
{
  float color[4];
  this->m_inputOperation->readSampled(color, x, y, sampler);
  copy_v3_v3(output, color);
}

/* ******** Value to Vector ******** */

ConvertValueToVectorOperation::ConvertValueToVectorOperation() : ConvertBaseOperation()
{
  this->addInputSocket(DataType::Value);
  this->addOutputSocket(DataType::Vector);
}

void ConvertValueToVectorOperation::executePixelSampled(float output[4],
                                                        float x,
                                                        float y,
                                                        PixelSampler sampler)
{
  float value;
  this->m_inputOperation->readSampled(&value, x, y, sampler);
  output[0] = output[1] = output[2] = value;
}

/* ******** Vector to Color ******** */

ConvertVectorToColorOperation::ConvertVectorToColorOperation() : ConvertBaseOperation()
{
  this->addInputSocket(DataType::Vector);
  this->addOutputSocket(DataType::Color);
}

void ConvertVectorToColorOperation::executePixelSampled(float output[4],
                                                        float x,
                                                        float y,
                                                        PixelSampler sampler)
{
  this->m_inputOperation->readSampled(output, x, y, sampler);
  output[3] = 1.0f;
}

/* ******** Vector to Value ******** */

ConvertVectorToValueOperation::ConvertVectorToValueOperation() : ConvertBaseOperation()
{
  this->addInputSocket(DataType::Vector);
  this->addOutputSocket(DataType::Value);
}

void ConvertVectorToValueOperation::executePixelSampled(float output[4],
                                                        float x,
                                                        float y,
                                                        PixelSampler sampler)
{
  float input[4];
  this->m_inputOperation->readSampled(input, x, y, sampler);
  output[0] = (input[0] + input[1] + input[2]) / 3.0f;
}

/* ******** Premul to Straight ******** */

ConvertPremulToStraightOperation::ConvertPremulToStraightOperation() : ConvertBaseOperation()
{
  this->addInputSocket(DataType::Color);
  this->addOutputSocket(DataType::Color);
}

void ConvertPremulToStraightOperation::executePixelSampled(float output[4],
                                                           float x,
                                                           float y,
                                                           PixelSampler sampler)
{
  float inputValue[4];
  float alpha;

  this->m_inputOperation->readSampled(inputValue, x, y, sampler);
  alpha = inputValue[3];

  if (fabsf(alpha) < 1e-5f) {
    zero_v3(output);
  }
  else {
    mul_v3_v3fl(output, inputValue, 1.0f / alpha);
  }

  /* never touches the alpha */
  output[3] = alpha;
}

/* ******** Straight to Premul ******** */

ConvertStraightToPremulOperation::ConvertStraightToPremulOperation() : ConvertBaseOperation()
{
  this->addInputSocket(DataType::Color);
  this->addOutputSocket(DataType::Color);
}

void ConvertStraightToPremulOperation::executePixelSampled(float output[4],
                                                           float x,
                                                           float y,
                                                           PixelSampler sampler)
{
  float inputValue[4];
  float alpha;

  this->m_inputOperation->readSampled(inputValue, x, y, sampler);
  alpha = inputValue[3];

  mul_v3_v3fl(output, inputValue, alpha);

  /* never touches the alpha */
  output[3] = alpha;
}

}  // namespace blender::compositor
