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

#include "COM_Rect.h"
#include "BLI_assert.h"
#include "COM_Buffer.h"
#include "COM_PixelsUtil.h"
#include "COM_defines.h"
#include <functional>
#include <tuple>

PixelsRect::PixelsRect(TmpBuffer *buf, int xmin, int xmax, int ymin, int ymax)
    : tmp_buffer(buf), is_single_elem(false), single_elem(nullptr), single_elem_chs(0)
{
  this->xmin = xmin;
  this->xmax = xmax;
  this->ymin = ymin;
  this->ymax = ymax;
  BLI_assert(BLI_rcti_is_valid(this));
  BLI_assert(!BLI_rcti_is_empty(this));
}
PixelsRect::PixelsRect(TmpBuffer *buf, const rcti &rect)
    : PixelsRect(buf, rect.xmin, rect.xmax, rect.ymin, rect.ymax)
{
}
PixelsRect::PixelsRect(
    float *single_elem, int single_elem_chs, int xmin, int xmax, int ymin, int ymax)
    : PixelsRect(nullptr, xmin, xmax, ymin, ymax)
{
  is_single_elem = true;
  this->single_elem = single_elem;
  this->single_elem_chs = single_elem_chs;
}
PixelsRect::PixelsRect(float *single_elem, int single_elem_chs, const rcti &rect)
    : PixelsRect(single_elem, single_elem_chs, rect.xmin, rect.xmax, rect.ymin, rect.ymax)
{
}

PixelsRect PixelsRect::toRect(const rcti &rect)
{
  if (is_single_elem) {
    return PixelsRect(single_elem, single_elem_chs, rect);
  }
  else {
    return PixelsRect(tmp_buffer, rect);
  }
}

PixelsImg PixelsRect::pixelsImg()
{
  if (is_single_elem) {
    return pixelsImgCustom(single_elem, 1, single_elem_chs, *this);
  }
  else {
    return pixelsImgCustom(
        tmp_buffer->host.buffer, tmp_buffer->host.width, tmp_buffer->elem_chs, *this);
  }
}

PixelsImg PixelsRect::pixelsImgCustom(float *buffer,
                                      int buffer_width,
                                      int n_channels,
                                      const rcti &rect)
{
  BLI_assert(BLI_rcti_is_valid(&rect));
  BLI_assert(!BLI_rcti_is_empty(&rect));
  float *buffer_y_start = buffer + (size_t)rect.ymin * buffer_width * n_channels;
  float *rect_start = buffer_y_start + (size_t)rect.xmin * n_channels;
  float *rect_end = buffer + ((size_t)rect.ymax - 1) * buffer_width * n_channels +
                    (size_t)rect.xmax * n_channels;

  int elem_chs = n_channels;
  size_t elem_bytes = elem_chs * sizeof(float);
  int row_elems = rect.xmax - rect.xmin;
  int col_elems = rect.ymax - rect.ymin;
  int row_chs = row_elems * elem_chs;
  int row_jump = (buffer_width - row_elems) * elem_chs;
  size_t row_bytes = row_chs * sizeof(float);
  int brow_elems = buffer_width;
  int brow_chs = brow_elems * elem_chs;
  size_t brow_bytes = brow_chs * sizeof(float);
  return PixelsImg{rect.xmin,        rect.ymin,        rect.xmax,        rect.ymax,
                   (float)rect.xmin, (float)rect.ymin, (float)rect.xmax, (float)rect.ymax,
                   buffer,           rect_start,       rect_end,         elem_chs,
                   elem_bytes,       row_elems,        col_elems,        row_chs,
                   row_jump,         row_bytes,        brow_elems,       brow_chs,
                   brow_bytes};
}