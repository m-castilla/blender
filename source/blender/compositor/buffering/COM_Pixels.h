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

#include <functional>
#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif

#include "COM_MathUtil.h"

class ComputeDevice;
class ComputeKernel;
class PixelsRect;
struct rcti;

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
#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:PixelsSampler")
#endif
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
  const bool is_single_elem;
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
  /* element channels used*/
  const int elem_chs;
  /* Per pixel element channels in the buffer (might be greater than used channels to allow
   * different image formats compatibility in all devices)*/
  const int belem_chs;
  /* used element bytes */
  const size_t elem_bytes;
  /* buffer element bytes */
  const size_t belem_bytes;
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

  /* row channels increment to be applied to jump an entire row independently of PixelsImg being a
   * single elem or a full buffer*/
  size_t brow_chs_incr;
  /* elem channels increment to be applied to jump an entire element independently of PixelsImg
   * being a single elem or a full buffer*/
  size_t belem_chs_incr;

  static PixelsImg create(float *buffer,
                          size_t buffer_row_bytes,
                          int n_channels,
                          int n_buffer_channels,
                          int width,
                          int height,
                          bool is_single_elem = false);
  static PixelsImg create(float *buffer,
                          size_t buffer_row_bytes,
                          int n_channels,
                          int n_buffer_channels,
                          const rcti &rect,
                          bool is_single_elem = false);
#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:PixelsImg")
#endif
} PixelsImg;

#endif
