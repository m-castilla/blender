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
#include "COM_BufferRecycler.h"
#include "COM_BufferUtil.h"
#include "COM_GlobalManager.h"
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
PixelsRect::PixelsRect(float single_elem[COM_NUM_CHANNELS_STD],
                       int n_used_chs,
                       int xmin,
                       int xmax,
                       int ymin,
                       int ymax)
    : PixelsRect(nullptr, xmin, xmax, ymin, ymax)
{
  is_single_elem = true;
  this->single_elem = single_elem;
  this->single_elem_chs = n_used_chs;
}
PixelsRect::PixelsRect(float single_elem[COM_NUM_CHANNELS_STD], int n_used_chs, const rcti &rect)
    : PixelsRect(single_elem, n_used_chs, rect.xmin, rect.xmax, rect.ymin, rect.ymax)
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
    return PixelsImg::create(single_elem,
                             COM_NUM_CHANNELS_STD * sizeof(float),
                             single_elem_chs,
                             COM_NUM_CHANNELS_STD,
                             *this,
                             true);
  }
  else {
    BLI_assert(tmp_buffer->host.state != HostMemoryState::NONE);
    BLI_assert(tmp_buffer->host.buffer != nullptr);

    PixelsImg img = PixelsImg::create(tmp_buffer->host.buffer,
                                      tmp_buffer->getBufferRowBytes(),
                                      tmp_buffer->elem_chs3,
                                      tmp_buffer->getBufferElemChs(),
                                      *this,
                                      false);
    /* When not mapped row_jump should always be 0 because we always create host buffers with 0
     * added pitch and divide images rects only vertically*/
    BLI_assert(tmp_buffer->host.state == HostMemoryState::MAP_FROM_DEVICE || img.row_jump == 0);

    return img;
  }
}

PixelsRect PixelsRect::duplicate()
{
  int width = getWidth();
  int height = getHeight();
  BufferRecycler *recycler = GlobalMan->BufferMan->recycler();
  TmpBuffer *dup_buffer = recycler->createTmpBuffer();
  recycler->takeNonStdRecycle(dup_buffer, width, height, getElemChs(), getBufferElemChs());
  PixelsRect dup_rect(dup_buffer, *this);
  PixelsUtil::copyEqualRects(dup_rect, *this);

  // assert there is no row jump, host recycles should not have it
  BLI_assert(dup_buffer->host.brow_bytes == dup_buffer->getMinBufferRowBytes());

  return dup_rect;
}
