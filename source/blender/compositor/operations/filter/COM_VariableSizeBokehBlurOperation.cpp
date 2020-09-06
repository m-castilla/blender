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

#include "COM_VariableSizeBokehBlurOperation.h"
#include "BLI_math.h"
#include "RE_pipeline.h"

#include "COM_kernel_cpu.h"

VariableSizeBokehBlurOperation::VariableSizeBokehBlurOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::COLOR,
                       InputResizeMode::NO_RESIZE);  // do not resize the bokeh image.
  this->addInputSocket(SocketType::VALUE);           // radius
#ifdef COM_DEFOCUS_SEARCH
  this->addInputSocket(COM_DT_COLOR,
                       COM_SC_NO_RESIZE);  // inverse search radius optimization structure.
#endif
  this->addOutputSocket(SocketType::COLOR);

  this->m_maxBlur = 32.0f;
  this->m_threshold = 1.0f;
  this->m_do_size_scale = false;
  this->m_max_size = FLT_MIN;
}

void VariableSizeBokehBlurOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_maxBlur);
  hashParam(m_threshold);
  hashParam(m_do_size_scale);
  hashParam((int)QualityStepHelper::getQuality());
}

void VariableSizeBokehBlurOperation::initExecution()
{
  this->m_max_size = FLT_MIN;
  QualityStepHelper::initExecution(QualityHelper::INCREASE);
  NodeOperation::initExecution();
}

static float getMaxSize(std::shared_ptr<PixelsRect> size, PixelsRect dst)
{
  float max_size = FLT_MIN;
  READ_DECL(size);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(size, dst_coords);

  float current_size = size_img.buffer[size_offset];
  if (current_size > max_size) {
    max_size = current_size;
  }

  CPU_LOOP_END;

  return max_size;
}

void VariableSizeBokehBlurOperation::execPixels(ExecutionManager &man)
{
  auto color = this->getInputOperation(0)->getPixels(this, man);
  auto bokeh = this->getInputOperation(1)->getPixels(this, man);
  auto size = this->getInputOperation(2)->getPixels(this, man);
  const float max_dim = CCL::max(m_width, m_height);
  const float scalar = this->m_do_size_scale ? (max_dim / 100.0f) : 1.0f;
  int quality_step = QualityStepHelper::getStep();
  PixelsSampler sampler = PixelsSampler{PixelInterpolation::NEAREST, PixelExtend::CLIP};
  auto cpu_write = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    if (ctx.current_pass == 0) {
      float max_size = getMaxSize(size, dst);
      mutex.lock();
      if (max_size > m_max_size) {
        m_max_size = max_size;
      }
      mutex.unlock();
    }
    else {
      int maxBlurScalar = (int)(m_max_size * scalar);
      CLAMP(maxBlurScalar, 1, m_maxBlur);

      READ_DECL(color);
      READ_DECL(bokeh);
      READ_DECL(size);
      WRITE_DECL(dst);

      CCL::float4 zero_f4 = CCL::make_float4_1(0.0f);
      CCL::float4 one_f4 = CCL::make_float4_1(1.0f);
      CCL::float4 multiplier_accum = zero_f4;
      CCL::float4 color_accum = zero_f4;
      CCL::float4 orig_color_pix;

      CPU_LOOP_START(dst);

#ifdef COM_DEFOCUS_SEARCH
      float search[4];
      this->m_inputSearchProgram->read(search,
                                       x / InverseSearchRadiusOperation::DIVIDER,
                                       y / InverseSearchRadiusOperation::DIVIDER,
                                       NULL);
      int minx = search[0];
      int miny = search[1];
      int maxx = search[2];
      int maxy = search[3];
#else
      int minx = CCL::max(dst_coords.x - maxBlurScalar, 0);
      int miny = CCL::max(dst_coords.y - maxBlurScalar, 0);
      int maxx = CCL::min(dst_coords.x + maxBlurScalar, (int)m_width);
      int maxy = CCL::min(dst_coords.y + maxBlurScalar, (int)m_height);
#endif

      COPY_COORDS(color, dst_coords);
      COPY_COORDS(size, dst_coords);
      READ_IMG4(color, orig_color_pix);
      READ_IMG1(size, size_pix);

      color_accum = orig_color_pix;
      multiplier_accum = one_f4;

      float size_center = size_pix.x * scalar;
      if (size_center > this->m_threshold) {
        SET_COORDS(size, minx, miny);
        int bokeh_x, bokeh_y;
        for (int ny = miny; ny < maxy; ny += quality_step) {
          float dy = ny - dst_coords.y;
          for (int nx = minx; nx < maxx; nx += quality_step) {
            if (nx != dst_coords.x || ny != dst_coords.y) {
              READ_IMG1(size, size_pix);
              float size_value = fminf(size_pix.x * scalar, size_center);
              if (size_value > this->m_threshold) {
                float dx = nx - dst_coords.x;
                if (size_value > fabsf(dx) && size_value > fabsf(dy)) {
                  bokeh_x = (float)(COM_BLUR_BOKEH_PIXELS / 2) +
                            (dx / size_value) * (float)((COM_BLUR_BOKEH_PIXELS / 2) - 1);
                  bokeh_y = (float)(COM_BLUR_BOKEH_PIXELS / 2) +
                            (dy / size_value) * (float)((COM_BLUR_BOKEH_PIXELS / 2) - 1);
                  SET_SAMPLE_COORDS(bokeh, bokeh_x, bokeh_y);
                  COPY_COORDS(color, size_coords);
                  SAMPLE_NEAREST4_CLIP(0, bokeh, sampler, bokeh_pix);
                  READ_IMG4(color, color_pix);
                  color_accum += bokeh_pix * color_pix;
                  multiplier_accum += bokeh_pix;
                }
              }
            }
            INCR_COORDS_X(size, quality_step);
            INCR_COORDS_X(color, quality_step);
          }
          INCR_COORDS_Y(size, quality_step);
          INCR_COORDS_Y(color, quality_step);
          UPDATE_COORDS_X(size, minx);
          UPDATE_COORDS_X(color, minx);
        }
      }

      color_pix = color_accum / multiplier_accum;

      /* blend in out values over the threshold, otherwise we get sharp, ugly transitions */
      if ((size_center > this->m_threshold) && (size_center < this->m_threshold * 2.0f)) {
        /* factor from 0-1 */
        float fac = (size_center - this->m_threshold) / this->m_threshold;
        color_pix = CCL::interp_f4f4(orig_color_pix, color_pix, fac);
      }

      WRITE_IMG4(dst, color_pix);

      CPU_LOOP_END;
    };
  };
  cpuWriteSeek(man, cpu_write);
}

#ifdef COM_DEFOCUS_SEARCH
// InverseSearchRadiusOperation
InverseSearchRadiusOperation::InverseSearchRadiusOperation() : NodeOperation()
{
  this->addInputSocket(COM_DT_VALUE, COM_SC_NO_RESIZE);  // radius
  this->addOutputSocket(COM_DT_COLOR);
  this->setComplex(true);
  this->m_inputRadius = NULL;
}

void InverseSearchRadiusOperation::initExecution()
{
  this->m_inputRadius = this->getInputSocketReader(0);
  NodeOperation::initExecution();
}

void *InverseSearchRadiusOperation::initializeTileData(rcti *rect)
{
  MemoryBuffer *data = new MemoryBuffer(COM_DT_COLOR, rect);
  float *buffer = data->getBuffer();
  int x, y;
  int width = this->m_inputRadius->getWidth();
  int height = this->m_inputRadius->getHeight();
  float temp[4];
  int offset = 0;
  for (y = rect->ymin; y < rect->ymax; y++) {
    for (x = rect->xmin; x < rect->xmax; x++) {
      int rx = x * DIVIDER;
      int ry = y * DIVIDER;
      buffer[offset] = MAX2(rx - m_maxBlur, 0);
      buffer[offset + 1] = MAX2(ry - m_maxBlur, 0);
      buffer[offset + 2] = MIN2(rx + DIVIDER + m_maxBlur, width);
      buffer[offset + 3] = MIN2(ry + DIVIDER + m_maxBlur, height);
      offset += 4;
    }
  }
  //  for (x = rect->xmin; x < rect->xmax ; x++) {
  //      for (y = rect->ymin; y < rect->ymax ; y++) {
  //          int rx = x * DIVIDER;
  //          int ry = y * DIVIDER;
  //          float radius = 0.0f;
  //          float maxx = x;
  //          float maxy = y;

  //          for (int x2 = 0 ; x2 < DIVIDER ; x2 ++) {
  //              for (int y2 = 0 ; y2 < DIVIDER ; y2 ++) {
  //                  this->m_inputRadius->read(temp, rx+x2, ry+y2, COM_PS_NEAREST);
  //                  if (radius < temp[0]) {
  //                      radius = temp[0];
  //                      maxx = x2;
  //                      maxy = y2;
  //                  }
  //              }
  //          }
  //          int impactRadius = ceil(radius / DIVIDER);
  //          for (int x2 = x - impactRadius ; x2 < x + impactRadius ; x2 ++) {
  //              for (int y2 = y - impactRadius ; y2 < y + impactRadius ; y2 ++) {
  //                  data->read(temp, x2, y2);
  //                  temp[0] = MIN2(temp[0], maxx);
  //                  temp[1] = MIN2(temp[1], maxy);
  //                  temp[2] = MAX2(temp[2], maxx);
  //                  temp[3] = MAX2(temp[3], maxy);
  //                  data->writePixel(x2, y2, temp);
  //              }
  //          }
  //      }
  //  }
  return data;
}

void InverseSearchRadiusOperation::executePixelChunk(float output[4], int x, int y, void *data)
{
  MemoryBuffer *buffer = (MemoryBuffer *)data;
  buffer->readNoCheck(output, x, y);
}

void InverseSearchRadiusOperation::deinitializeTileData(rcti *rect, void *data)
{
  if (data) {
    MemoryBuffer *mb = (MemoryBuffer *)data;
    delete mb;
  }
}

void InverseSearchRadiusOperation::deinitExecution()
{
  this->m_inputRadius = NULL;
}

void InverseSearchRadiusOperation::determineResolution(unsigned int resolution[2],
                                                       unsigned int preferredResolution[2])
{
  NodeOperation::determineResolution(resolution, preferredResolution);
  resolution[0] = resolution[0] / DIVIDER;
  resolution[1] = resolution[1] / DIVIDER;
}

bool InverseSearchRadiusOperation::determineDependingAreaOfInterest(
    rcti *input, ReadBufferOperation *readOperation, rcti *output)
{
  rcti newRect;
  newRect.ymin = input->ymin * DIVIDER - m_maxBlur;
  newRect.ymax = input->ymax * DIVIDER + m_maxBlur;
  newRect.xmin = input->xmin * DIVIDER - m_maxBlur;
  newRect.xmax = input->xmax * DIVIDER + m_maxBlur;
  return NodeOperation::determineDependingAreaOfInterest(&newRect, readOperation, output);
}
#endif
