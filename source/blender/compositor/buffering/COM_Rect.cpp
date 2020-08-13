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
    return pixelsImgCustom(
        single_elem, true, single_elem_chs * sizeof(float), single_elem_chs, *this);
  }
  else {
    BLI_assert(tmp_buffer->host.state != HostMemoryState::NONE);
    BLI_assert(tmp_buffer->host.buffer != nullptr);
    PixelsImg img = pixelsImgCustom(
        tmp_buffer->host.buffer, false, tmp_buffer->host.brow_bytes, tmp_buffer->elem_chs, *this);
    /* When not mapped row_jump should always be 0 because we always create host buffers with 0
     * added pitch and divide images rects only vertically*/
    BLI_assert(tmp_buffer->host.state == HostMemoryState::MAP_FROM_DEVICE || img.row_jump == 0);

    return img;
  }
}

PixelsImg PixelsRect::pixelsImgCustom(
    float *buffer, bool is_single_elem, size_t buffer_row_bytes, int n_channels, const rcti &rect)
{
  BLI_assert(BLI_rcti_is_valid(&rect));
  BLI_assert(!BLI_rcti_is_empty(&rect));
  BLI_assert(buffer_row_bytes > 0);

  int elem_chs = n_channels;
  size_t elem_bytes = elem_chs * sizeof(float);

  size_t brow_elems = buffer_row_bytes / elem_bytes;
  size_t brow_chs = brow_elems * elem_chs;

  float *buffer_y_start = is_single_elem ? buffer :
                                           buffer + (size_t)rect.ymin * brow_elems * n_channels;
  float *rect_start = is_single_elem ? buffer : buffer_y_start + (size_t)rect.xmin * n_channels;
  float *rect_end = is_single_elem ? buffer + elem_chs :
                                     buffer + ((size_t)rect.ymax - 1) * brow_elems * n_channels +
                                         (size_t)rect.xmax * n_channels;
  BLI_assert(rect_end > rect_start);

  int row_elems = rect.xmax - rect.xmin;
  int col_elems = rect.ymax - rect.ymin;
  int row_chs = row_elems * elem_chs;
  int row_jump = is_single_elem ? 0 : brow_chs - row_chs;
  BLI_assert(row_jump >= 0);
  size_t row_bytes = row_chs * sizeof(float);

  BLI_assert(!(row_jump == 0 && !is_single_elem) ||
             (rect_end - rect_start) == (size_t)row_elems * col_elems * elem_chs);

  return PixelsImg{rect.xmin,        rect.ymin,        rect.xmax,        rect.ymax,
                   (float)rect.xmin, (float)rect.ymin, (float)rect.xmax, (float)rect.ymax,
                   buffer,           rect_start,       rect_end,         elem_chs,
                   elem_bytes,       row_elems,        col_elems,        row_chs,
                   row_jump,         row_bytes,        brow_elems,       brow_chs,
                   buffer_row_bytes};
}