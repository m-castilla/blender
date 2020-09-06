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
#include "COM_Pixels.h"
#include "BLI_assert.h"
#include "BLI_rect.h"
#include "COM_BufferUtil.h"
#include "COM_Rect.h"
#include "MEM_guardedalloc.h"

void PixelInterpolationForeach(std::function<void(PixelInterpolation)> func)
{
  for (int i = 0; i <= static_cast<int>(PixelInterpolation::BILINEAR); i++) {
    func(static_cast<PixelInterpolation>(i));
  }
}

void PixelExtendForeach(std::function<void(PixelExtend)> func)
{
  for (int i = 0; i <= static_cast<int>(PixelExtend::MIRROR); i++) {
    func(static_cast<PixelExtend>(i));
  }
}

PixelsImg PixelsImg::create(float *buffer,
                            size_t buffer_row_bytes,
                            int n_used_channels,
                            int n_buffer_channels,
                            int width,
                            int height,
                            bool is_single_elem)
{
  rcti rect = {0, width, 0, height};
  return PixelsImg::create(
      buffer, buffer_row_bytes, n_used_channels, n_buffer_channels, rect, is_single_elem);
}

PixelsImg PixelsImg::create(float *buffer,
                            size_t buffer_row_bytes,
                            int n_used_channels,
                            int n_buffer_channels,
                            const rcti &rect,
                            bool is_single_elem)
{
  BLI_assert(BLI_rcti_is_valid(&rect));
  BLI_assert(!BLI_rcti_is_empty(&rect));
  BLI_assert(buffer_row_bytes > 0);

  int row_elems = rect.xmax - rect.xmin;
  int col_elems = rect.ymax - rect.ymin;

  int elem_chs = n_used_channels;
  int belem_chs = n_buffer_channels;
  size_t elem_bytes = elem_chs * sizeof(float);
  size_t belem_bytes = belem_chs * sizeof(float);

  size_t brow_elems = buffer_row_bytes / belem_bytes;
  size_t brow_chs = brow_elems * belem_chs;

  float *buffer_y_start = is_single_elem ? buffer : buffer + (size_t)rect.ymin * brow_chs;
  float *rect_start = is_single_elem ? buffer : buffer_y_start + (size_t)rect.xmin * belem_chs;
  float *rect_end = is_single_elem ? buffer + belem_chs :
                                     buffer + ((size_t)rect.ymax - 1) * brow_chs +
                                         (size_t)rect.xmax * belem_chs;
  BLI_assert(rect_end > rect_start);
  int row_chs = row_elems * belem_chs;
  int row_jump = is_single_elem ? 0 : brow_chs - row_chs;
  BLI_assert(row_jump >= 0);
  size_t row_bytes = row_chs * sizeof(float);

  BLI_assert(!(row_jump == 0 && !is_single_elem) ||
             (rect_end - rect_start) == (ptrdiff_t)row_elems * col_elems * belem_chs);

  size_t brow_chs_incr = is_single_elem ? 0 : brow_chs;
  size_t belem_chs_incr = is_single_elem ? 0 : belem_chs;

  return PixelsImg{is_single_elem,   rect.xmin,        rect.ymin,        rect.xmax,
                   rect.ymax,        (float)rect.xmin, (float)rect.ymin, (float)rect.xmax,
                   (float)rect.ymax, buffer,           rect_start,       rect_end,
                   elem_chs,         belem_chs,        elem_bytes,       belem_bytes,
                   row_elems,        col_elems,        row_chs,          row_jump,
                   row_bytes,        brow_elems,       brow_chs,         buffer_row_bytes,
                   brow_chs_incr,    belem_chs_incr};
}
