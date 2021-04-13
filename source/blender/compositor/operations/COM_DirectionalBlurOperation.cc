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

#include "COM_DirectionalBlurOperation.h"
#include "COM_PixelUtil.h"

#include "BLI_math.h"

#include "RE_pipeline.h"

namespace blender::compositor {

DirectionalBlurOperation::DirectionalBlurOperation()
{
  this->addInputSocket(DataType::Color);
  this->addOutputSocket(DataType::Color);
  flags.complex = true;
  flags.open_cl = true;
}

void DirectionalBlurOperation::initExecution()
{
  QualityStepHelper::initExecution(COM_QH_INCREASE);
  const float angle = this->m_data->angle;
  const float zoom = this->m_data->zoom;
  const float spin = this->m_data->spin;
  const float iterations = this->m_data->iter;
  const float distance = this->m_data->distance;
  const float center_x = this->m_data->center_x;
  const float center_y = this->m_data->center_y;
  const float width = getWidth();
  const float height = getHeight();

  const float a = angle;
  const float itsc = 1.0f / powf(2.0f, (float)iterations);
  float D;

  D = distance * sqrtf(width * width + height * height);
  this->m_center_x_pix = center_x * width;
  this->m_center_y_pix = center_y * height;

  this->m_tx = itsc * D * cosf(a);
  this->m_ty = -itsc * D * sinf(a);
  this->m_sc = itsc * zoom;
  this->m_rot = itsc * spin;
}

void DirectionalBlurOperation::execPixelsMultiCPU(const rcti &render_rect,
                                                  CPUBuffer<float> &output,
                                                  blender::Span<const CPUBuffer<float> *> inputs,
                                                  ExecutionSystem *exec_system,
                                                  int current_pass)
{
  auto &input = *inputs[0];
  const int iterations = pow(2.0f, this->m_data->iter);
  float col[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  float col2[4] = {0.0f, 0.0f, 0.0f, 0.0f};

  for (int y = render_rect.ymin; y < render_rect.ymax; y++) {
    int x = render_rect.xmin;
    float *output_elem = output.getElem(x, y);
    for (; x < render_rect.xmax; x++) {
      PixelUtil::readBilinear(input, col2, x, y);
      float ltx = this->m_tx;
      float lty = this->m_ty;
      float lsc = this->m_sc;
      float lrot = this->m_rot;
      /* blur the image */
      for (int i = 0; i < iterations; i++) {
        const float cs = cosf(lrot), ss = sinf(lrot);
        const float isc = 1.0f / (1.0f + lsc);

        const float v = isc * (y - this->m_center_y_pix) + lty;
        const float u = isc * (x - this->m_center_x_pix) + ltx;

        PixelUtil::readBilinear(input,
                                col,
                                cs * u + ss * v + this->m_center_x_pix,
                                cs * v - ss * u + this->m_center_y_pix);

        add_v4_v4(col2, col);

        /* double transformations */
        ltx += this->m_tx;
        lty += this->m_ty;
        lrot += this->m_rot;
        lsc += this->m_sc;
      }

      mul_v4_v4fl(output_elem, col2, 1.0f / (iterations + 1));

      output_elem += output.elem_jump;
    }
  }
}

void DirectionalBlurOperation::execPixelsGPU(const rcti &render_rect,
                                             GPUBuffer &output,
                                             blender::Span<const GPUBuffer *> inputs,
                                             ExecutionSystem *exec_system)
{
  const int iterations = pow(2.0f, this->m_data->iter);
  float ltxy[2] = {this->m_tx, this->m_ty};
  float centerpix[2] = {this->m_center_x_pix, this->m_center_y_pix};
  const GPUBuffer *input = inputs[0];
  exec_system->execWorkGPU(
      render_rect, "directionalBlurKernel", [=, &output](ComputeKernel *kernel) {
        kernel->addBufferArg(input);
        kernel->addBufferArg(&output);
        kernel->addIntArg(iterations);
        kernel->addFloatArg(m_sc);
        kernel->addFloatArg(m_rot);
        kernel->addFloat2Arg(ltxy);
        kernel->addFloat2Arg(centerpix);
      });
}

}  // namespace blender::compositor
