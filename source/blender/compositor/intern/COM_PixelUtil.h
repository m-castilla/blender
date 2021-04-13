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
 * Copyright 2021, Blender Foundation.
 */

#pragma once

#include "BLI_math.h"
#include "BLI_rect.h"
#include "COM_Buffer.h"

namespace blender::compositor::PixelUtil {

inline void wrap_pixel(const CPUBuffer<float> &buf,
                       int &x,
                       int &y,
                       MemoryBufferExtend extend_x,
                       MemoryBufferExtend extend_y)
{
  int w = buf.width;
  int h = buf.height;
  x = x - buf.rect.xmin;
  y = y - buf.rect.ymin;

  switch (extend_x) {
    case COM_MB_CLIP:
      break;
    case COM_MB_EXTEND:
      if (x < 0) {
        x = 0;
      }
      if (x >= w) {
        x = w;
      }
      break;
    case COM_MB_REPEAT:
      x = (x >= 0.0f ? (x % w) : (x % w) + w);
      break;
  }

  switch (extend_y) {
    case COM_MB_CLIP:
      break;
    case COM_MB_EXTEND:
      if (y < 0) {
        y = 0;
      }
      if (y >= h) {
        y = h;
      }
      break;
    case COM_MB_REPEAT:
      y = (y >= 0.0f ? (y % h) : (y % h) + h);
      break;
  }
}

inline void wrap_pixel(const CPUBuffer<float> &buf,
                       float &x,
                       float &y,
                       MemoryBufferExtend extend_x,
                       MemoryBufferExtend extend_y)
{
  float w = (float)buf.width;
  float h = (float)buf.height;
  x = x - buf.rect.xmin;
  y = y - buf.rect.ymin;

  switch (extend_x) {
    case COM_MB_CLIP:
      break;
    case COM_MB_EXTEND:
      if (x < 0) {
        x = 0.0f;
      }
      if (x >= w) {
        x = w;
      }
      break;
    case COM_MB_REPEAT:
      x = fmodf(x, w);
      break;
  }

  switch (extend_y) {
    case COM_MB_CLIP:
      break;
    case COM_MB_EXTEND:
      if (y < 0) {
        y = 0.0f;
      }
      if (y >= h) {
        y = h;
      }
      break;
    case COM_MB_REPEAT:
      y = fmodf(y, h);
      break;
  }
}

inline void read(const CPUBuffer<float> &buf,
                 float *result,
                 int x,
                 int y,
                 MemoryBufferExtend extend_x = COM_MB_CLIP,
                 MemoryBufferExtend extend_y = COM_MB_CLIP)
{
  bool clip_x = (extend_x == COM_MB_CLIP && (x < buf.rect.xmin || x >= buf.rect.xmax));
  bool clip_y = (extend_y == COM_MB_CLIP && (y < buf.rect.ymin || y >= buf.rect.ymax));
  if (clip_x || clip_y) {
    /* clip result outside rect is zero */
    memset(result, 0, buf.elem_bytes);
  }
  else {
    int u = x;
    int v = y;
    PixelUtil::wrap_pixel(buf, u, v, extend_x, extend_y);
    const int offset = buf.row_jump * y + x * buf.elem_jump;
    const float *buffer = &buf[offset];
    memcpy(result, buffer, buf.elem_bytes);
  }
}

inline void readNoCheck(const CPUBuffer<float> &buf,
                        float *result,
                        int x,
                        int y,
                        MemoryBufferExtend extend_x = COM_MB_CLIP,
                        MemoryBufferExtend extend_y = COM_MB_CLIP)
{
  int u = x;
  int v = y;

  PixelUtil::wrap_pixel(buf, u, v, extend_x, extend_y);
  const int offset = buf.row_jump * v + u * buf.elem_jump;

  BLI_assert(offset >= 0);
  BLI_assert(offset < buf.used_bytes);
  BLI_assert(!(extend_x == COM_MB_CLIP && (u < buf.rect.xmin || u >= buf.rect.xmax)) &&
             !(extend_y == COM_MB_CLIP && (v < buf.rect.ymin || v >= buf.rect.ymax)));
  const float *buffer = &buf[offset];
  memcpy(result, buffer, buf.elem_bytes);
}

void writePixel(CPUBuffer<float> &buf, int x, int y, const float color[4]);
void addPixel(CPUBuffer<float> &buf, int x, int y, const float color[4]);
inline void readBilinear(const CPUBuffer<float> &buf,
                         float *result,
                         float x,
                         float y,
                         MemoryBufferExtend extend_x = COM_MB_CLIP,
                         MemoryBufferExtend extend_y = COM_MB_CLIP)
{
  float u = x;
  float v = y;
  PixelUtil::wrap_pixel(buf, u, v, extend_x, extend_y);
  if ((extend_x != COM_MB_REPEAT && (u < 0.0f || u >= buf.width)) ||
      (extend_y != COM_MB_REPEAT && (v < 0.0f || v >= buf.height))) {
    copy_vn_fl(result, buf.elem_chs, 0.0f);
    return;
  }
  BLI_bilinear_interpolation_wrap_fl(&buf[0],
                                     result,
                                     buf.width,
                                     buf.height,
                                     buf.elem_chs,
                                     u,
                                     v,
                                     extend_x == COM_MB_REPEAT,
                                     extend_y == COM_MB_REPEAT);
}

void readEWA(const CPUBuffer<float> &buf,
             float *result,
             const float uv[2],
             const float derivatives[2][2]);

};  // namespace blender::compositor::PixelUtil
