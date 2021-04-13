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

void ConvertBaseOperation::execPixelsMultiCPU(const rcti &render_rect,
                                              CPUBuffer<float> &output,
                                              blender::Span<const CPUBuffer<float> *> inputs,
                                              ExecutionSystem *exec_system,
                                              int current_pass)
{
  auto &input = *inputs[0];
  int x_end = render_rect.xmax;
  int y_end = render_rect.ymax;
  int x = render_rect.xmin;
  for (int y = render_rect.ymin; y < y_end; y++) {
    auto output_elem = output.getElem(x, y);
    auto input_elem = input.getElem(x, y);
    auto output_end_elem = output.getElem(x_end, y_end);
    execPixelsRowCPU(output_elem, output_end_elem, output.elem_jump, input_elem, input.elem_jump);
  }
}

/* ******** Value to Color ******** */

ConvertValueToColorOperation::ConvertValueToColorOperation() : ConvertBaseOperation()
{
  this->addInputSocket(DataType::Value);
  this->addOutputSocket(DataType::Color);
}

void ConvertValueToColorOperation::execPixelsRowCPU(
    float *output, const float *output_end, int output_jump, const float *input, int input_jump)
{
  while (output < output_end) {
    output[0] = output[1] = output[2] = input[0];
    output[3] = 1.0f;

    output += output_jump;
    input += input_jump;
  }
}

/* ******** Color to Value ******** */

ConvertColorToValueOperation::ConvertColorToValueOperation() : ConvertBaseOperation()
{
  this->addInputSocket(DataType::Color);
  this->addOutputSocket(DataType::Value);
}

void ConvertColorToValueOperation::execPixelsRowCPU(
    float *output, const float *output_end, int output_jump, const float *input, int input_jump)
{
  while (output < output_end) {
    output[0] = (input[0] + input[1] + input[2]) / 3.0f;

    output += output_jump;
    input += input_jump;
  }
}

/* ******** Color to Vector ******** */

ConvertColorToVectorOperation::ConvertColorToVectorOperation() : ConvertBaseOperation()
{
  this->addInputSocket(DataType::Color);
  this->addOutputSocket(DataType::Vector);
}

void ConvertColorToVectorOperation::execPixelsRowCPU(
    float *output, const float *output_end, int output_jump, const float *input, int input_jump)
{
  while (output < output_end) {
    copy_v3_v3(output, input);

    output += output_jump;
    input += input_jump;
  }
}

/* ******** Value to Vector ******** */

ConvertValueToVectorOperation::ConvertValueToVectorOperation() : ConvertBaseOperation()
{
  this->addInputSocket(DataType::Value);
  this->addOutputSocket(DataType::Vector);
}

void ConvertValueToVectorOperation::execPixelsRowCPU(
    float *output, const float *output_end, int output_jump, const float *input, int input_jump)
{
  while (output < output_end) {
    output[0] = output[1] = output[2] = input[0];

    output += output_jump;
    input += input_jump;
  }
}

/* ******** Vector to Color ******** */

ConvertVectorToColorOperation::ConvertVectorToColorOperation() : ConvertBaseOperation()
{
  this->addInputSocket(DataType::Vector);
  this->addOutputSocket(DataType::Color);
}

void ConvertVectorToColorOperation::execPixelsRowCPU(
    float *output, const float *output_end, int output_jump, const float *input, int input_jump)
{
  while (output < output_end) {
    copy_v3_v3(output, input);
    output[3] = 1.0f;

    output += output_jump;
    input += input_jump;
  }
}

/* ******** Vector to Value ******** */

ConvertVectorToValueOperation::ConvertVectorToValueOperation() : ConvertBaseOperation()
{
  this->addInputSocket(DataType::Vector);
  this->addOutputSocket(DataType::Value);
}

void ConvertVectorToValueOperation::execPixelsRowCPU(
    float *output, const float *output_end, int output_jump, const float *input, int input_jump)
{
  while (output < output_end) {
    output[0] = (input[0] + input[1] + input[2]) / 3.0f;

    output += output_jump;
    input += input_jump;
  }
}

/* ******** Premul to Straight ******** */

ConvertPremulToStraightOperation::ConvertPremulToStraightOperation() : ConvertBaseOperation()
{
  this->addInputSocket(DataType::Color);
  this->addOutputSocket(DataType::Color);
}

void ConvertPremulToStraightOperation::execPixelsRowCPU(
    float *output, const float *output_end, int output_jump, const float *input, int input_jump)
{
  while (output < output_end) {
    float alpha = input[3];

    if (fabsf(alpha) < 1e-5f) {
      zero_v3(output);
    }
    else {
      mul_v3_v3fl(output, input, 1.0f / alpha);
    }

    /* never touches the alpha */
    output[3] = alpha;

    output += output_jump;
    input += input_jump;
  }
}

/* ******** Straight to Premul ******** */

ConvertStraightToPremulOperation::ConvertStraightToPremulOperation() : ConvertBaseOperation()
{
  this->addInputSocket(DataType::Color);
  this->addOutputSocket(DataType::Color);
}

void ConvertStraightToPremulOperation::execPixelsRowCPU(
    float *output, const float *output_end, int output_jump, const float *input, int input_jump)
{
  while (output < output_end) {
    float alpha = input[3];

    mul_v3_v3fl(output, input, alpha);

    /* never touches the alpha */
    output[3] = alpha;

    output += output_jump;
    input += input_jump;
  }
}

}  // namespace blender::compositor
