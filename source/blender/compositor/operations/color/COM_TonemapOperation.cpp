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

#include "COM_TonemapOperation.h"
#include "BLI_math.h"
#include "BLI_utildefines.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"
#include "IMB_colormanagement.h"

TonemapOperation::TonemapOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
  this->m_tone = nullptr;
  this->m_avg = nullptr;
  this->m_sum = nullptr;
}

void TonemapOperation::initExecution()
{
  m_avg = new AvgLogLum{0, 0, 0, {0, 0, 0, 0}, 0};
  m_sum = new SumLogLum{0, 0, {0, 0, 0, 0}, 0, 0};
  NodeOperation::initExecution();
}

void TonemapOperation::deinitExecution()
{
  if (m_avg) {
    delete m_avg;
    m_avg = nullptr;
  }
  if (m_sum) {
    delete m_sum;
    m_sum = nullptr;
  }
  NodeOperation::deinitExecution();
}

void TonemapOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_tone->a);
  hashParam(m_tone->c);
  hashParam(m_tone->f);
  hashParam(m_tone->gamma);
  hashParam(m_tone->key);
  hashParam(m_tone->m);
  hashParam(m_tone->offset);
  hashParam(m_tone->type);
}

void TonemapOperation::calcAverage(std::shared_ptr<PixelsRect> color,
                                   PixelsRect &dst,
                                   const WriteRectContext &ctx)
{
  READ_DECL(color);
  WRITE_DECL(dst);

  /* Calculate average */
  float lsum = 0.0f;
  float maxl = -1e10f, minl = 1e10f;
  float Lav = 0.0f;
  float cav[4] = {0.0f, 0.0f, 0.0f, 0.0f};
  float *color_buf = color_img.buffer;

  CPU_LOOP_START(dst);

  COPY_COORDS(color, dst_coords);
  float L = IMB_colormanagement_get_luminance(&color_buf[color_offset]);
  Lav += L;
  add_v3_v3(cav, &color_buf[color_offset]);
  lsum += logf(MAX2(L, 0.0f) + 1e-5f);
  maxl = (L > maxl) ? L : maxl;
  minl = (L < minl) ? L : minl;

  CPU_LOOP_END;

  m_mutex.lock();
  add_v3_v3(m_sum->cav_sum, cav);
  m_sum->l_sum += lsum;
  m_sum->lav_sum += Lav;
  m_sum->maxl = maxl > m_sum->maxl ? maxl : m_sum->maxl;
  m_sum->minl = minl < m_sum->minl ? minl : m_sum->minl;
  m_sum->n_rects++;
  if (m_sum->n_rects == ctx.n_rects) {
    int total_area = color->getWidth() * color->getHeight();
    float sc = 1.0f / total_area;
    mul_v3_v3fl(m_avg->cav, m_sum->cav_sum, sc);
    float final_maxl = log((double)m_sum->maxl + 1e-5);
    float final_minl = log((double)m_sum->minl + 1e-5);
    m_avg->lav = m_sum->lav_sum * sc;
    float avl = m_sum->l_sum * sc;
    m_avg->auto_key = final_maxl > final_minl ? (final_maxl - avl) / (final_maxl - final_minl) :
                                                1.0f;
    float al = exp((double)avl);
    m_avg->al = (al == 0.0f) ? 0.0f : (m_tone->key / al);
    m_avg->igm = (m_tone->gamma == 0.0f) ? 1 : (1.0f / m_tone->gamma);
  }
  m_mutex.unlock();
}

void TonemapOperation::execPixels(ExecutionManager &man)
{
  auto color = this->getInputOperation(0)->getPixels(this, man);
  auto cpu_write = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    if (ctx.current_pass == 0) {
      calcAverage(color, dst, ctx);
    }
    else {
      READ_DECL(color);
      WRITE_DECL(dst);

      CPU_LOOP_START(dst);

      COPY_COORDS(color, dst_coords);

      READ_IMG4(color, color_pix);

      const float alpha = color_pix.w;
      color_pix *= m_avg->al;
      CCL::float4 c_offset = color_pix + m_tone->offset;

      color_pix.x /= c_offset.x == 0.0f ? 1.0f : c_offset.x;
      color_pix.y /= c_offset.y == 0.0f ? 1.0f : c_offset.y;
      color_pix.z /= c_offset.z == 0.0f ? 1.0f : c_offset.z;

      const float igm = m_avg->igm;
      if (igm != 0.0f) {
        color_pix.x = powf(fmaxf(color_pix.x, 0.0f), igm);
        color_pix.y = powf(fmaxf(color_pix.y, 0.0f), igm);
        color_pix.z = powf(fmaxf(color_pix.z, 0.0f), igm);
      }
      color_pix.w = alpha;

      WRITE_IMG4(dst, color_pix);

      CPU_LOOP_END;
    }
  };
  cpuWriteSeek(man, cpu_write);
}

void PhotoreceptorTonemapOperation::execPixels(ExecutionManager &man)
{
  auto color = this->getInputOperation(0)->getPixels(this, man);
  auto cpu_write = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    if (ctx.current_pass == 0) {
      calcAverage(color, dst, ctx);
    }
    else {
      const float f = expf(-m_tone->f);
      const float m = (m_tone->m > 0.0f) ? m_tone->m : (0.3f + 0.7f * powf(m_avg->auto_key, 1.4f));
      const float ic = 1.0f - m_tone->c, ia = 1.0f - m_tone->a;

      READ_DECL(color);
      WRITE_DECL(dst);

      CPU_LOOP_START(dst);

      COPY_COORDS(color, dst_coords);

      READ_IMG4(color, color_pix);

      const float alpha = color_pix.w;

      const float L = IMB_colormanagement_get_luminance((float *)&color_pix);
      float I_l = color_pix.x + ic * (L - color_pix.x);
      float I_g = m_avg->cav[0] + ic * (m_avg->lav - m_avg->cav[0]);
      float I_a = I_l + ia * (I_g - I_l);
      color_pix.x /= (color_pix.x + powf(f * I_a, m));
      I_l = color_pix.y + ic * (L - color_pix.y);
      I_g = m_avg->cav[1] + ic * (m_avg->lav - m_avg->cav[1]);
      I_a = I_l + ia * (I_g - I_l);
      color_pix.y /= (color_pix.y + powf(f * I_a, m));
      I_l = color_pix.z + ic * (L - color_pix.z);
      I_g = m_avg->cav[2] + ic * (m_avg->lav - m_avg->cav[2]);
      I_a = I_l + ia * (I_g - I_l);
      color_pix.z /= (color_pix.z + powf(f * I_a, m));

      color_pix.w = alpha;

      WRITE_IMG4(dst, color_pix);

      CPU_LOOP_END;
    }
  };
  cpuWriteSeek(man, cpu_write);
}
