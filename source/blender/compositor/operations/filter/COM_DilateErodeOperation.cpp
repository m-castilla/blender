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

#include "COM_DilateErodeOperation.h"
#include "COM_BufferUtil.h"
#include "COM_ComputeKernel.h"
#include "COM_PixelsUtil.h"
#include "MEM_guardedalloc.h"
#include <algorithm>

#include "COM_kernel_cpu.h"

using namespace std::placeholders;
// DilateErode Distance Threshold
DilateErodeThresholdOperation::DilateErodeThresholdOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::VALUE);
  this->m_inset = 0.0f;
  this->m__switch = 0.5f;
  this->m_distance = 0.0f;
  m_scope = 0;
}
void DilateErodeThresholdOperation::initExecution()
{
  if (this->m_distance < 0.0f) {
    this->m_scope = -this->m_distance + this->m_inset;
  }
  else {
    if (this->m_inset * 2 > this->m_distance) {
      this->m_scope = std::max(this->m_inset * 2 - this->m_distance, this->m_distance);
    }
    else {
      this->m_scope = this->m_distance;
    }
  }
  if (this->m_scope < 3) {
    this->m_scope = 3;
  }
  NodeOperation::initExecution();
}

void DilateErodeThresholdOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_inset);
  hashParam(m__switch);
  hashParam(m_distance);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel dilateErodeThresholdOp(CCL_WRITE(dst),
                                  CCL_READ(input),
                                  const float distance,
                                  const float orig_min_dist,
                                  const float inset,
                                  const float sw,
                                  const int scope,
                                  const int width,
                                  const int height)
{
  READ_DECL(input);
  WRITE_DECL(dst);
  float pixel_value;

  CPU_LOOP_START(dst);

  const int minx = max(dst_coords.x - scope, 0);
  const int miny = max(dst_coords.y - scope, 0);
  const int maxx = min(dst_coords.x + scope, width);
  const int maxy = min(dst_coords.y + scope, height);

  float min_dist = orig_min_dist;
  COPY_COORDS(input, dst_coords);
  READ_IMG1(input, input_pix);
  if (input_pix.x > sw) {
    SET_COORDS(input, minx, miny);
    while (input_coords.y < maxy) {
      const float dy = input_coords.y - dst_coords.y;
      while (input_coords.x < maxx) {
        READ_IMG1(input, input_pix);
        if (input_pix.x < sw) {
          const float dx = input_coords.x - dst_coords.x;
          const float dis = dx * dx + dy * dy;
          min_dist = fminf(min_dist, dis);
        }
        INCR1_COORDS_X(input);
      }
      INCR1_COORDS_Y(input);
      UPDATE_COORDS_X(input, minx);
    }
    pixel_value = -sqrtf(min_dist);
  }
  else {
    SET_COORDS(input, minx, miny);
    while (input_coords.y < maxy) {
      const float dy = input_coords.y - dst_coords.y;
      while (input_coords.x < maxx) {
        READ_IMG1(input, input_pix);
        if (input_pix.x > sw) {
          const float dx = input_coords.x - dst_coords.x;
          const float dis = dx * dx + dy * dy;
          min_dist = fminf(min_dist, dis);
        }
        INCR1_COORDS_X(input);
      }
      INCR1_COORDS_Y(input);
      UPDATE_COORDS_X(input, minx);
    }
    pixel_value = sqrtf(min_dist);
  }

  /* Set pixel result*/
  if (distance > 0.0f) {
    const float delta = distance - pixel_value;
    if (delta >= 0.0f) {
      if (delta >= inset) {
        input_pix.x = 1.0f;
      }
      else {
        input_pix.x = delta / inset;
      }
    }
    else {
      input_pix.x = 0.0f;
    }
  }
  else {
    const float delta = -distance + pixel_value;
    if (delta < 0.0f) {
      if (delta < -inset) {
        input_pix.x = 1.0f;
      }
      else {
        input_pix.x = (-delta) / inset;
      }
    }
    else {
      input_pix.x = 0.0f;
    }
  }

  WRITE_IMG1(dst, input_pix);

  CPU_LOOP_END;
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void DilateErodeThresholdOperation::execPixels(ExecutionManager &man)
{
  auto input = getInputOperation(0)->getPixels(this, man);
  const float rd = m_scope * m_scope;
  const float min_dist = rd * 2;

  auto cpu_write = std::bind(CCL::dilateErodeThresholdOp,
                             _1,
                             input,
                             m_distance,
                             min_dist,
                             m_inset,
                             m__switch,
                             m_scope,
                             getWidth(),
                             getHeight());
  computeWriteSeek(man, cpu_write, "dilateErodeThresholdOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input);
    kernel->addFloatArg(m_distance);
    kernel->addFloatArg(min_dist);
    kernel->addFloatArg(m_inset);
    kernel->addFloatArg(m__switch);
    kernel->addIntArg(m_scope);
    kernel->addIntArg(getWidth());
    kernel->addIntArg(getHeight());
  });
}

// Dilate Distance
DilateDistanceOperation::DilateDistanceOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::VALUE);
  this->m_distance = 0.0f;
  m_scope = 0;
}

void DilateDistanceOperation::initExecution()
{
  this->m_scope = this->m_distance;
  if (this->m_scope < 3) {
    this->m_scope = 3;
  }
  NodeOperation::initExecution();
}

void DilateDistanceOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_distance);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel dilateDistanceOp(CCL_WRITE(dst),
                            CCL_READ(input),
                            const float min_dist,
                            const int scope,
                            const int width,
                            const int height)
{
  READ_DECL(input);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  const int minx = max(dst_coords.x - scope, 0);
  const int miny = max(dst_coords.y - scope, 0);
  const int maxx = min(dst_coords.x + scope, width);
  const int maxy = min(dst_coords.y + scope, height);

  float value = 0.0f;
  SET_COORDS(input, minx, miny);
  while (input_coords.y < maxy) {
    const float dy = input_coords.y - dst_coords.y;
    while (input_coords.x < maxx) {
      const float dx = input_coords.x - dst_coords.x;
      const float dis = dx * dx + dy * dy;
      if (dis <= min_dist) {
        READ_IMG1(input, input_pix);
        value = max(input_pix.x, value);
      }
      INCR1_COORDS_X(input);
    }
    INCR1_COORDS_Y(input)
    UPDATE_COORDS_X(input, minx);
  }

  input_pix.x = value;
  WRITE_IMG1(dst, input_pix);

  CPU_LOOP_END;
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void DilateDistanceOperation::execPixels(ExecutionManager &man)
{
  auto input = getInputOperation(0)->getPixels(this, man);
  const float min_dist = m_distance * m_distance;

  auto cpu_write = std::bind(
      CCL::dilateDistanceOp, _1, input, min_dist, m_scope, getWidth(), getHeight());
  computeWriteSeek(man, cpu_write, "dilateDistanceOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input);
    kernel->addFloatArg(min_dist);
    kernel->addIntArg(m_scope);
    kernel->addIntArg(getWidth());
    kernel->addIntArg(getHeight());
  });
}

// Erode Distance
ErodeDistanceOperation::ErodeDistanceOperation() : DilateDistanceOperation()
{
  /* pass */
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel erodeDistanceOp(CCL_WRITE(dst),
                           CCL_READ(input),
                           const float min_dist,
                           const int scope,
                           const int last_x,
                           const int last_y)
{
  READ_DECL(input);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  const int minx = max(dst_coords.x - scope, 0);
  const int miny = max(dst_coords.y - scope, 0);
  const int maxx = min(dst_coords.x + scope, last_x);
  const int maxy = min(dst_coords.y + scope, last_y);

  float value = 1.0f;
  SET_COORDS(input, minx, miny);
  while (input_coords.y < maxy) {
    const float dy = input_coords.y - dst_coords.y;
    while (input_coords.x < maxx) {
      const float dx = input_coords.x - dst_coords.x;
      const float dis = dx * dx + dy * dy;
      if (dis <= min_dist) {
        READ_IMG1(input, input_pix);
        value = min(input_pix.x, value);
      }
      INCR1_COORDS_X(input);
    }
    INCR1_COORDS_Y(input)
    UPDATE_COORDS_X(input, minx);
  }

  input_pix.x = value;
  WRITE_IMG1(dst, input_pix);

  CPU_LOOP_END;
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void ErodeDistanceOperation::execPixels(ExecutionManager &man)
{
  auto input = getInputOperation(0)->getPixels(this, man);
  const float min_dist = m_distance * m_distance;
  const int last_x = getWidth() - 1;
  const int last_y = getHeight() - 1;

  auto cpu_write = std::bind(CCL::erodeDistanceOp, _1, input, min_dist, m_scope, last_x, last_y);
  computeWriteSeek(man, cpu_write, "erodeDistanceOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input);
    kernel->addFloatArg(min_dist);
    kernel->addIntArg(m_scope);
    kernel->addIntArg(last_x);
    kernel->addIntArg(last_y);
  });
}

// Dilate step
DilateStepOperation::DilateStepOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::VALUE);
  m_iterations = 0;
}

void DilateStepOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_iterations);
}

void DilateStepOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);

  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    PixelsImg dst_img = dst.pixelsImg();
    PixelsImg color_img = color->pixelsImg();

    int x, y, i;
    int width = color_img.row_elems;
    int height = color_img.col_elems;
    float *buffer = color_img.buffer;

    int half_window = this->m_iterations;
    int window = half_window * 2 + 1;

    int xmin = std::max(0, dst_img.start_x - half_window);
    int ymin = std::max(0, dst_img.start_y - half_window);
    int xmax = std::min(width, dst_img.end_x + half_window);
    int ymax = std::min(height, dst_img.end_y + half_window);

    int bwidth = dst_img.end_x - dst_img.start_x;
    int bheight = dst_img.end_y - dst_img.start_y;

    // Note: rectf buffer has original tilesize width, but new height.
    // We have to calculate the additional rows in the first pass,
    // to have valid data available for the second pass.
    int rectf_w = xmax - xmin;
    int rectf_h = ymax - ymin;
    float *rectf = (float *)MEM_callocN(sizeof(float) * rectf_h * rectf_w, "dilate erode cache");

    // temp holds maxima for every step in the algorithm, buf holds a
    // single row or column of input values, padded with FLT_MAX's to
    // simplify the logic.
    float *temp = (float *)MEM_mallocN(sizeof(float) * (2 * (size_t)window - 1),
                                       "dilate erode temp");
    float *buf = (float *)MEM_mallocN(
        sizeof(float) * (std::max(bwidth, bheight) + 5 * (size_t)half_window), "dilate erode buf");

    // The following is based on the van Herk/Gil-Werman algorithm for morphology operations.
    // first pass, horizontal dilate/erode
    for (y = ymin; y < ymax; y++) {
      for (x = 0; x < bwidth + 5 * half_window; x++) {
        buf[x] = -FLT_MAX;
      }
      for (x = xmin; x < xmax; x++) {
        buf[x - dst_img.start_x + window - 1] = buffer[(y * width + x)];
      }

      for (i = 0; i < (bwidth + 3 * half_window) / window; i++) {
        int start = (i + 1) * window - 1;

        temp[window - 1] = buf[start];
        for (x = 1; x < window; x++) {
          temp[window - 1 - x] = std::max(temp[window - x], buf[start - x]);
          temp[window - 1 + x] = std::max(temp[window + x - 2], buf[start + x]);
        }

        start = half_window + (i - 1) * window + 1;
        for (x = -std::min(0, start); x < window - std::max(0, start + window - bwidth); x++) {
          rectf[bwidth * (y - ymin) + (start + x)] = std::max(temp[x], temp[x + window - 1]);
        }
      }
    }

    // second pass, vertical dilate/erode
    for (x = 0; x < bwidth; x++) {
      for (y = 0; y < bheight + 5 * half_window; y++) {
        buf[y] = -FLT_MAX;
      }
      for (y = ymin; y < ymax; y++) {
        buf[y - dst_img.start_y + window - 1] = rectf[(y - ymin) * bwidth + x];
      }

      for (i = 0; i < (bheight + 3 * half_window) / window; i++) {
        int start = (i + 1) * window - 1;

        temp[window - 1] = buf[start];
        for (y = 1; y < window; y++) {
          temp[window - 1 - y] = std::max(temp[window - y], buf[start - y]);
          temp[window - 1 + y] = std::max(temp[window + y - 2], buf[start + y]);
        }

        start = half_window + (i - 1) * window + 1;
        for (y = -std::min(0, start); y < window - std::max(0, start + window - bheight); y++) {
          rectf[bwidth * (y + start + (dst_img.start_y - ymin)) + x] = std::max(
              temp[y], temp[y + window - 1]);
        }
      }
    }

    MEM_freeN(temp);
    MEM_freeN(buf);

    // center the rectf result into dst
    int xmin_added_window = dst_img.start_x - xmin;
    int xmax_added_window = xmax - dst_img.end_x;
    int ymin_added_window = dst_img.start_y - ymin;
    int ymax_added_window = ymax - dst_img.end_y;
    auto rectf_tmp = BufferUtil::createUnmanagedTmpBuffer(rectf, rectf_w, rectf_h, 1, true);
    PixelsRect rectf_pr = PixelsRect(rectf_tmp.get(),
                                     xmin_added_window,
                                     rectf_w - xmax_added_window,
                                     ymin_added_window,
                                     rectf_h - ymax_added_window);
    PixelsUtil::copyEqualRects(dst, rectf_pr);

    MEM_freeN(rectf);
  };
  return cpuWriteSeek(man, cpuWrite);
}

// Erode step
ErodeStepOperation::ErodeStepOperation() : DilateStepOperation()
{
  /* pass */
}

void ErodeStepOperation::execPixels(ExecutionManager &man)
{
  auto input = getInputOperation(0)->getPixels(this, man);

  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    PixelsImg dst_img = dst.pixelsImg();

    READ_DECL(input);

    int x, y, i;
    const int width = input_img.row_elems;
    const int height = input_img.col_elems;

    const int half_window = this->m_iterations;
    const int window = half_window * 2 + 1;

    const int xmin = std::max(0, dst_img.start_x - half_window);
    const int ymin = std::max(0, dst_img.start_y - half_window);
    const int xmax = std::min(width, dst_img.end_x + half_window);
    const int ymax = std::min(height, dst_img.end_y + half_window);

    const int bwidth = dst_img.end_x - dst_img.start_x;
    const int bheight = dst_img.end_y - dst_img.start_y;

    // Note: rectf buffer has original tilesize width, but new height.
    // We have to calculate the additional rows in the first pass,
    // to have valid data available for the second pass.
    int rectf_w = xmax - xmin;
    int rectf_h = ymax - ymin;
    float *rectf = (float *)MEM_callocN(sizeof(float) * rectf_h * rectf_w, "dilate erode cache");

    // temp holds maxima for every step in the algorithm, buf holds a
    // single row or column of input values, padded with FLT_MAX's to
    // simplify the logic.
    float *temp = (float *)MEM_mallocN(sizeof(float) * (2 * (size_t)window - 1),
                                       "dilate erode temp");
    float *buf = (float *)MEM_mallocN(
        sizeof(float) * (std::max(bwidth, bheight) + 5 * (size_t)half_window), "dilate erode buf");

    // The following is based on the van Herk/Gil-Werman algorithm for morphology operations.
    // first pass, horizontal dilate/erode
    SET_COORDS(input, xmin, ymin);
    for (y = ymin; y < ymax; y++) {
      for (x = 0; x < bwidth + 5 * half_window; x++) {
        buf[x] = FLT_MAX;
      }
      for (x = xmin; x < xmax; x++) {
        buf[x - dst_img.start_x + window - 1] = input_img.buffer[input_offset];
        INCR1_COORDS_X(input);
      }

      for (i = 0; i < (bwidth + 3 * half_window) / window; i++) {
        int start = (i + 1) * window - 1;

        temp[window - 1] = buf[start];
        for (x = 1; x < window; x++) {
          temp[window - 1 - x] = std::min(temp[window - x], buf[start - x]);
          temp[window - 1 + x] = std::min(temp[window + x - 2], buf[start + x]);
        }

        start = half_window + (i - 1) * window + 1;
        for (x = -std::min(0, start); x < window - std::max(0, start + window - bwidth); x++) {
          rectf[bwidth * (y - ymin) + (start + x)] = std::min(temp[x], temp[x + window - 1]);
        }
      }
      INCR1_COORDS_Y(input);
      UPDATE_COORDS_X(input, xmin);
    }

    // second pass, vertical dilate/erode
    for (x = 0; x < bwidth; x++) {
      for (y = 0; y < bheight + 5 * half_window; y++) {
        buf[y] = FLT_MAX;
      }
      for (y = ymin; y < ymax; y++) {
        buf[y - dst_img.start_y + window - 1] = rectf[(y - ymin) * bwidth + x];
      }

      for (i = 0; i < (bheight + 3 * half_window) / window; i++) {
        int start = (i + 1) * window - 1;

        temp[window - 1] = buf[start];
        for (y = 1; y < window; y++) {
          temp[window - 1 - y] = std::min(temp[window - y], buf[start - y]);
          temp[window - 1 + y] = std::min(temp[window + y - 2], buf[start + y]);
        }

        start = half_window + (i - 1) * window + 1;
        for (y = -std::min(0, start); y < window - std::max(0, start + window - bheight); y++) {
          rectf[bwidth * (y + start + (dst_img.start_y - ymin)) + x] = std::min(
              temp[y], temp[y + window - 1]);
        }
      }
    }

    MEM_freeN(temp);
    MEM_freeN(buf);

    // center the rectf result into dst
    int xmin_added_window = dst_img.start_x - xmin;
    int xmax_added_window = xmax - dst_img.end_x;
    int ymin_added_window = dst_img.start_y - ymin;
    int ymax_added_window = ymax - dst_img.end_y;
    auto rectf_tmp = BufferUtil::createUnmanagedTmpBuffer(rectf, rectf_w, rectf_h, 1, true);
    PixelsRect rectf_pr = PixelsRect(rectf_tmp.get(),
                                     xmin_added_window,
                                     rectf_w - xmax_added_window,
                                     ymin_added_window,
                                     rectf_h - ymax_added_window);
    PixelsUtil::copyEqualRects(dst, rectf_pr);

    MEM_freeN(rectf);
  };
  return cpuWriteSeek(man, cpuWrite);
}
