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
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_GlobalManager.h"
#include "COM_kernel_cpu.h"

using namespace std::placeholders;
DirectionalBlurOperation::DirectionalBlurOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
}

void DirectionalBlurOperation::initExecution()
{
  QualityStepHelper::initExecution(QualityHelper::INCREASE);
  const float angle = this->m_data->angle;
  const float zoom = this->m_data->zoom;
  const float spin = this->m_data->spin;
  const float user_iterations = this->m_data->iter;
  const float distance = this->m_data->distance;
  const float center_x = this->m_data->center_x;
  const float center_y = this->m_data->center_y;
  const float width = getWidth();
  const float height = getHeight();

  const float a = angle;
  const float iterations = powf(2.0f, (float)user_iterations);
  const float itsc = 1.0f / iterations;

  this->m_iterations = iterations;

  float D = distance * sqrtf(width * width + height * height);
  this->m_pix_center_x = center_x * width;
  this->m_pix_center_y = center_y * height;

  this->m_tx = itsc * D * cosf(a);
  this->m_ty = -itsc * D * sinf(a);
  this->m_sc = itsc * zoom;
  this->m_rot = itsc * spin;
  NodeOperation::initExecution();
}

void DirectionalBlurOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_data->angle);
  hashParam(m_data->zoom);
  hashParam(m_data->spin);
  hashParam(m_data->iter);
  hashParam(m_data->center_x);
  hashParam(m_data->center_y);
  hashParam(m_data->distance);
  /* Wrap is not used */
  // hashParam(m_data->wrap);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel directionalBlurOp(CCL_WRITE(dst),
                             CCL_READ(color),
                             CCL_SAMPLER(sampler),
                             const int iterations,
                             ccl_constant float *it_table,
                             const int it_table_row_length,
                             const float4 one_by_iterations_plus_one_f4,
                             const float pix_center_x,
                             const float pix_center_y,
                             const float tx,
                             const float ty)
{
  READ_DECL(color);
  WRITE_DECL(dst);
  float ltx;
  float lty;
  float4 result_pix;

  CPU_LOOP_START(dst);

  result_pix = make_float4_1(0.0f);
  ltx = tx;
  lty = ty;

  SET_SAMPLE_COORDS(color, dst_coords.x, dst_coords.y);

  SAMPLE_BILINEAR4_CLIP(0, color, sampler, result_pix);

  /* blur the image */
  int it_table_end = iterations * it_table_row_length;
  float u, v, uu, vv;
  for (int it_table_row = 0; it_table_row < it_table_end; it_table_row += it_table_row_length) {
    const float cos_rot = it_table[it_table_row];
    const float sin_rot = it_table[it_table_row + 1];
    const float inverse_sc = it_table[it_table_row + 2];

    u = inverse_sc * (dst_coords.x - pix_center_x) + ltx;
    v = inverse_sc * (dst_coords.y - pix_center_y) + lty;

    uu = cos_rot * u + sin_rot * v + pix_center_x;
    vv = cos_rot * v - sin_rot * u + pix_center_y;

    SET_SAMPLE_COORDS(color, uu, vv);
    SAMPLE_BILINEAR4_CLIP(1, color, sampler, color_pix);

    result_pix += color_pix;
    ltx += tx;
    lty += ty;
  }

  result_pix *= one_by_iterations_plus_one_f4;

  WRITE_IMG4(dst, result_pix);

  CPU_LOOP_END;
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void DirectionalBlurOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  float *it_table = nullptr;
  int it_table_row_length = 3;
  CCL::float4 one_by_iterations_plus_one_f4;
  TmpBuffer *it_table_buf = nullptr;
  auto recycler = GlobalMan->BufferMan->recycler();
  if (man.canExecPixels()) {
    float one_by_iterations_plus_one = 1.0f / (m_iterations + 1);
    one_by_iterations_plus_one_f4 = CCL::make_float4_1(one_by_iterations_plus_one);

    it_table_buf = recycler->createTmpBuffer();
    recycler->takeRecycle(
        BufferRecycleType::HOST_CLEAR, it_table_buf, m_iterations, it_table_row_length, 1);
    it_table = it_table_buf->host.buffer;

    int it_table_end = m_iterations * it_table_row_length;
    float lsc = this->m_sc;
    float lrot = this->m_rot;
    for (int it_table_row = 0; it_table_row < it_table_end; it_table_row += it_table_row_length) {
      it_table[it_table_row] = cosf(lrot);
      it_table[it_table_row + 1] = sinf(lrot);
      it_table[it_table_row + 2] = 1.0f / (1.0f + lsc);

      /* double transformations */
      lrot += this->m_rot;
      lsc += this->m_sc;
    }
  }

  PixelsSampler sampler = {PixelInterpolation::BILINEAR, PixelExtend::CLIP};
  auto cpu_write = std::bind(CCL::directionalBlurOp,
                             _1,
                             color,
                             sampler,
                             m_iterations,
                             it_table,
                             it_table_row_length,
                             one_by_iterations_plus_one_f4,
                             m_pix_center_x,
                             m_pix_center_y,
                             m_tx,
                             m_ty);
  computeWriteSeek(man, cpu_write, "directionalBlurOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addSamplerArg(sampler);
    kernel->addIntArg(m_iterations);
    kernel->addFloatCArrayArg(it_table, m_iterations * it_table_row_length);
    kernel->addIntArg(it_table_row_length);
    kernel->addFloat4Arg(one_by_iterations_plus_one_f4);
    kernel->addFloatArg(m_pix_center_x);
    kernel->addFloatArg(m_pix_center_y);
    kernel->addFloatArg(m_tx);
    kernel->addFloatArg(m_ty);
  });

  if (it_table_buf) {
    recycler->giveRecycle(it_table_buf);
  }
}
