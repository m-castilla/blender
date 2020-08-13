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

#ifndef __COM_PIXELS_H__
#define __COM_PIXELS_H__

#include "BLI_assert.h"
#include "BLI_math.h"
#include "COM_MathUtil.h"
#include <functional>
#include <memory>
#include <string>

class ComputeDevice;
class ComputeKernel;
class PixelsRect;

enum class PixelInterpolation { NEAREST, BILINEAR };
void PixelInterpolationForeach(std::function<void(PixelInterpolation)> func);

enum class PixelExtend { UNCHECKED, CLIP, EXTEND, REPEAT, MIRROR };
void PixelExtendForeach(std::function<void(PixelExtend)> func);

typedef struct PixelsSampler {
  PixelInterpolation filter;
  PixelExtend extend;
  bool const operator==(const PixelsSampler &o) const
  {
    return filter == o.filter && extend == o.extend;
  }
} PixelsSampler;

/* PixelsSampler default hash function. Mainly for being used by
                                   default in std::unordermap */
namespace std {
template<> struct hash<PixelsSampler> {
  size_t operator()(const PixelsSampler &k) const
  {
    hash<int> hasher{};
    size_t hash = hasher(static_cast<int>(k.filter));
    size_t extend_hash = hasher(static_cast<int>(k.extend));
    MathUtil::hashCombine(hash, extend_hash);
    return hash;
  }
};
}  // namespace std

typedef struct PixelsImg {
  const int start_x, start_y;
  const int end_x, end_y;
  const float start_xf, start_yf;
  const float end_xf, end_yf;

  /* raw host buffer pointer */
  float *buffer;
  /* First pixel of the rect*/
  float *start;
  /* rect end */
  const float *const end;
  /* element channels (pixel increment)*/
  const int elem_chs;
  /* element bytes */
  const size_t elem_bytes;
  /* rect row elements (rect width)*/
  const int row_elems;
  /* rect row elements (rect height)*/
  const int col_elems;
  /* rect row channels (rect row elems * elem channels)*/
  const int row_chs;
  /* used for jumping to the start of the next rect row when you are at the end of the previous one
   * (needed because buffer row might be bigger than rect row)*/
  const int row_jump;
  /* whole rect row bytes*/
  const size_t row_bytes;
  /* buffer row elements  */
  const size_t brow_elems;
  /* buffer row channels (buffer row elems * elem channels)*/
  const size_t brow_chs;
  /* buffer row bytes (buffer row elems * elem channels)*/
  const size_t brow_bytes;

 private:
  inline bool checkExtend(float *dst, const PixelsSampler &sampler, float &u, float &v)
  {
    if (u < start_xf || u >= end_xf) {
      switch (sampler.extend) {
        case PixelExtend::UNCHECKED:
          BLI_assert(!"Given coordinates are out of buffer range");
          break;
        case PixelExtend::CLIP:
          /* clip result outside rect is zero */
          memset(dst, 0, elem_bytes);
          return true;
          break;
        case PixelExtend::EXTEND:
          u = u < start_xf ? 0.0f : end_xf - 1.0f;
          break;
        case PixelExtend::REPEAT:
          u = fmodf(u - start_xf, row_elems) + start_xf;
          break;
        case PixelExtend::MIRROR:
          u = (end_xf - 1.0f) - (fmodf(u - start_xf, row_elems) + start_xf);
          break;
      }
    }
    if (v < start_yf || v >= end_yf) {
      switch (sampler.extend) {
        case PixelExtend::UNCHECKED:
          BLI_assert(!"Given coordinates are out of buffer range");
          break;
        case PixelExtend::CLIP:
          /* clip result outside rect is zero */
          memset(dst, 0, elem_bytes);
          return true;
          break;
        case PixelExtend::EXTEND:
          v = v < start_yf ? 0.0f : end_yf - 1.0f;
          break;
        case PixelExtend::REPEAT:
          v = fmodf(v - start_yf, col_elems) + start_yf;
          break;
        case PixelExtend::MIRROR:
          v = (end_yf - 1.0f) - (fmodf(v - start_yf, col_elems) + start_yf);
          break;
      }
    }

    return false;
  }

  inline bool checkExtend(float *dst, const PixelsSampler &sampler, int &x, int &y)
  {
    if (x < start_x || x >= end_x) {
      switch (sampler.extend) {
        case PixelExtend::UNCHECKED:
          BLI_assert(!"Given coordinates are out of buffer range");
          break;
        case PixelExtend::CLIP:
          /* clip result outside rect is zero */
          memset(dst, 0, elem_bytes);
          return true;
          break;
        case PixelExtend::EXTEND:
          x = x < start_x ? 0 : end_x - 1;
          break;
        case PixelExtend::REPEAT:
          x = ((x - start_x) % row_elems) + start_x;
          break;
        case PixelExtend::MIRROR:
          x = (end_x - 1) - (((x - start_x) % row_elems) + start_x);
          break;
      }
    }
    if (y < start_y || y >= end_y) {
      switch (sampler.extend) {
        case PixelExtend::UNCHECKED:
          BLI_assert(!"Given coordinates are out of buffer range");
          break;
        case PixelExtend::CLIP:
          /* clip result outside rect is zero */
          memset(dst, 0, elem_bytes);
          return true;
          break;
        case PixelExtend::EXTEND:
          y = y < start_y ? 0 : end_y - 1;
          break;
        case PixelExtend::REPEAT:
          y = ((y - start_y) % col_elems) + start_y;
          break;
        case PixelExtend::MIRROR:
          y = (end_y - 1) - (((y - start_y) % col_elems) + start_y);
          break;
      }
    }

    return false;
  }

  inline void read_nearest(float *dst, const PixelsSampler &sampler, int x, int y)
  {
    if (checkExtend(dst, sampler, x, y)) {
      return;
    }
    memcpy(dst, start + y * (size_t)brow_chs + x * (size_t)elem_chs, elem_bytes);
  }

  inline void read_nearest(float *dst, const PixelsSampler &sampler, float u, float v)
  {
    if (checkExtend(dst, sampler, u, v)) {
      return;
    }
    memcpy(dst, start + ((size_t)v) * (size_t)brow_chs + (size_t)u * (size_t)elem_chs, elem_bytes);
  }

  inline void read_bilinear(float *dst, const PixelsSampler &sampler, float u, float v)
  {
    if (checkExtend(dst, sampler, u, v)) {
      return;
    }

    float a, b;
    float a_b, ma_b, a_mb, ma_mb;
    int y1, y2, x1, x2;

    x1 = floor(u);
    x2 = ceil(u);
    y1 = floor(v);
    y2 = ceil(v);

    /* check ceil boundaries extend. x1 and y1 were already checked at the start, floor would not
     * change the result of "check(dst, u, v)"*/
    if (checkExtend(dst, sampler, x2, y2)) {
      return;
    }

    const float *row1, *row2, *row3, *row4;
    float empty[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    /* sample including outside of edges of image */
    if (x1 < 0 || y1 < 0) {
      row1 = empty;
    }
    else {
      row1 = start + y1 * (size_t)brow_chs + (size_t)elem_chs * x1;
    }

    if (x1 < 0 || y2 > col_elems - 1) {
      row2 = empty;
    }
    else {
      row2 = start + y2 * (size_t)brow_chs + (size_t)elem_chs * x1;
    }

    if (x2 > row_elems - 1 || y1 < 0) {
      row3 = empty;
    }
    else {
      row3 = start + y1 * (size_t)brow_chs + (size_t)elem_chs * x2;
    }

    if (x2 > row_elems - 1 || y2 > col_elems - 1) {
      row4 = empty;
    }
    else {
      row4 = start + y2 * (size_t)brow_chs + (size_t)elem_chs * x2;
    }

    a = u - floorf(u);
    b = v - floorf(v);
    a_b = a * b;
    ma_b = (1.0f - a) * b;
    a_mb = a * (1.0f - b);
    ma_mb = (1.0f - a) * (1.0f - b);

    if (elem_chs == 1) {
      dst[0] = ma_mb * row1[0] + a_mb * row3[0] + ma_b * row2[0] + a_b * row4[0];
    }
    else if (elem_chs == 3) {
      dst[0] = ma_mb * row1[0] + a_mb * row3[0] + ma_b * row2[0] + a_b * row4[0];
      dst[1] = ma_mb * row1[1] + a_mb * row3[1] + ma_b * row2[1] + a_b * row4[1];
      dst[2] = ma_mb * row1[2] + a_mb * row3[2] + ma_b * row2[2] + a_b * row4[2];
    }
    else {
      dst[0] = ma_mb * row1[0] + a_mb * row3[0] + ma_b * row2[0] + a_b * row4[0];
      dst[1] = ma_mb * row1[1] + a_mb * row3[1] + ma_b * row2[1] + a_b * row4[1];
      dst[2] = ma_mb * row1[2] + a_mb * row3[2] + ma_b * row2[2] + a_b * row4[2];
      dst[3] = ma_mb * row1[3] + a_mb * row3[3] + ma_b * row2[3] + a_b * row4[3];
    }
  }

 public:
  inline void sample(float *dst, const PixelsSampler &sampler, float u, float v)
  {
    switch (sampler.filter) {
      case PixelInterpolation::NEAREST:
        read_nearest(dst, sampler, u, v);
        break;
      case PixelInterpolation::BILINEAR:
        read_bilinear(dst, sampler, u, v);
        break;
      default:
        BLI_assert(!"Non implemented sampler filter");
        break;
    }
  }

  inline void sample(float *dst, const PixelsSampler &sampler, int x, int y)
  {
    switch (sampler.filter) {
      case PixelInterpolation::NEAREST:
        read_nearest(dst, sampler, x, y);
        break;
      case PixelInterpolation::BILINEAR:
        read_bilinear(dst, sampler, x, y);
        break;
      default:
        BLI_assert(!"Non implemented sampler filter");
        break;
    }
  }

} PixelsImg;

#endif
