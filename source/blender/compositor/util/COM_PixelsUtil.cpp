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
 * Copyright 2020, Blender Foundation.
 */

#include "COM_PixelsUtil.h"
#include "BLI_assert.h"
#include "COM_Bmp.h"
#include "COM_Buffer.h"
#include "COM_BufferUtil.h"
#include "COM_ExecutionManager.h"
#include "COM_NodeOperation.h"
#include "COM_Rect.h"
#include "COM_defines.h"
#include <string.h>

int PixelsUtil::getNChannels(DataType dataType)
{
  switch (dataType) {
    case DataType::VALUE:
      return COM_NUM_CHANNELS_VALUE;
    case DataType::VECTOR:
      return COM_NUM_CHANNELS_VECTOR;
    case DataType::COLOR:
      return COM_NUM_CHANNELS_COLOR;
    default:
      BLI_assert(!"non implemented DataType");
      throw;
  }
}

SocketType PixelsUtil::dataToSocketType(DataType d)
{
  switch (d) {
    case DataType::COLOR:
      return SocketType::COLOR;
    case DataType::VECTOR:
      return SocketType::VECTOR;
    case DataType::VALUE:
      return SocketType::VALUE;
    default:
      BLI_assert(!"Unimplemented data type");
      return (SocketType)0;
  }
}

void PixelsUtil::copyEqualRects(PixelsRect &wr1, PixelsRect &rr1)
{
  // write rect should never be single element
  BLI_assert(!wr1.is_single_elem);
  if (rr1.is_single_elem) {
    setRectElem(wr1, rr1.single_elem);
  }
  else {
    PixelsImg w1 = wr1.pixelsImg();
    PixelsImg r1 = rr1.pixelsImg();

    BLI_assert(w1.row_bytes == r1.row_bytes && w1.row_jump >= 0 && r1.row_jump >= 0);
    if (w1.row_jump == 0 && r1.row_jump == 0) {
      memcpy(w1.start, r1.start, (w1.end - w1.start) * sizeof(float));
    }
    else {
      float *w1_cur = w1.start;
      float *r1_cur = r1.start;
      while (w1_cur < w1.end) {
        memcpy(w1_cur, r1_cur, r1.row_bytes);
        w1_cur += w1.brow_chs;
        r1_cur += r1.brow_chs;
      }
    }
  }
}

void PixelsUtil::copyEqualRectsNChannels(PixelsRect &wr1, PixelsRect &rr1, int n_channels)
{
  // write rect should never be single element
  BLI_assert(!wr1.is_single_elem);
  if (rr1.is_single_elem) {
    setRectElem(wr1, rr1.single_elem, n_channels);
  }
  else if (wr1.tmp_buffer->elem_chs == n_channels &&
           wr1.tmp_buffer->elem_chs == rr1.tmp_buffer->elem_chs) {
    copyEqualRects(wr1, rr1);
  }
  else {
    PixelsImg w1 = wr1.pixelsImg();
    PixelsImg r1 = rr1.pixelsImg();
    size_t elem_bytes = n_channels * sizeof(float);
    BLI_assert(w1.elem_bytes >= elem_bytes && r1.elem_bytes >= elem_bytes);
    float *w1_cur = w1.start;
    float *r1_cur = r1.start;
    float *w1_row_end = w1.start + w1.row_chs;
    while (w1_cur < w1.end) {
      while (w1_cur < w1_row_end) {
        memcpy(w1_cur, r1_cur, elem_bytes);

        w1_cur += w1.elem_chs;
        r1_cur += r1.elem_chs;
      }
      w1_cur += w1.row_jump;
      r1_cur += r1.row_jump;
      w1_row_end += w1.brow_chs;
    }
  }
}

void PixelsUtil::copyEqualRectsChannel(PixelsRect &wr1,
                                       int wr_channel,
                                       PixelsRect &cr1,
                                       int cr_channel)
{
  // write rect should never be single element
  BLI_assert(!wr1.is_single_elem);
  if (cr1.is_single_elem) {
    setRectChannel(wr1, wr_channel, cr1.single_elem[cr_channel]);
  }
  else {
    PixelsImg w1 = wr1.pixelsImg();
    PixelsImg c1 = cr1.pixelsImg();
    BLI_assert(w1.row_elems == c1.row_elems && w1.elem_chs > wr_channel &&
               c1.elem_chs > cr_channel);
    float *w1_cur = w1.start + wr_channel;
    float *c1_cur = c1.start + cr_channel;
    float *w1_row_end = w1.start + w1.row_chs;
    while (w1_cur < w1.end) {
      while (w1_cur < w1_row_end) {
        *w1_cur = *c1_cur;

        w1_cur += w1.elem_chs;
        c1_cur += c1.elem_chs;
      }
      w1_cur += w1.row_jump;
      w1_row_end += w1.brow_chs;
      c1_cur += c1.row_jump;
    }
  }
}

void PixelsUtil::setRectChannel(PixelsRect &wr1, int channel, float channel_value)
{
  // write rect should never be single element
  BLI_assert(!wr1.is_single_elem);
  PixelsImg w1 = wr1.pixelsImg();
  BLI_assert(w1.elem_chs > channel);
  float *w1_cur = w1.start + channel;
  float *w1_row_end = w1.start + w1.row_chs;
  while (w1_cur < w1.end) {
    while (w1_cur < w1_row_end) {
      *w1_cur = channel_value;

      w1_cur += w1.elem_chs;
    }
    w1_cur += w1.row_jump;
    w1_row_end += w1.brow_chs;
  }
}

void PixelsUtil::setRectElem(PixelsRect &wr1, const float *elem)
{
  setRectElem(wr1, elem, wr1.tmp_buffer->elem_chs);
}

void PixelsUtil::setRectElem(PixelsRect &wr1, const float elem)
{
  setRectElem(wr1, &elem, 1);
}

void PixelsUtil::setRectElem(PixelsRect &wr1, const float *elem, int n_channels)
{
  // write rect should never be single element
  BLI_assert(!wr1.is_single_elem);
  PixelsImg w1 = wr1.pixelsImg();
  BLI_assert(w1.elem_bytes >= sizeof(float) && w1.elem_bytes % sizeof(float) == 0);
  BLI_assert(w1.elem_chs >= n_channels);
  float *w1_cur = w1.start;
  float *w1_row_end = w1.start + w1.row_chs;
  size_t chs_bytes = n_channels * sizeof(float);
  while (w1_cur < w1.end) {
    while (w1_cur < w1_row_end) {
      memcpy(w1_cur, elem, chs_bytes);
      w1_cur += w1.elem_chs;
    }
    w1_cur += w1.row_jump;
    w1_row_end += w1.brow_chs;
  }
}

#if defined(COM_DEBUG) || defined(DEBUG)

void PixelsUtil::saveAsImage(TmpBuffer *buf, std::string filename, ExecutionManager &man)
{
  PixelsRect rect(buf, 0, buf->width, 0, buf->height);
  saveAsImage(rect, filename, man);
}

void PixelsUtil::saveAsImage(PixelsRect &rect, std::string filename, ExecutionManager &man)
{
  auto buf = rect.tmp_buffer;
  bool copy_from_device = false;
  if (buf->device.state == DeviceMemoryState::FILLED) {
    if (buf->host.state == HostMemoryState::CLEARED) {
      copy_from_device = true;
    }
    else if (buf->host.state == HostMemoryState::NONE) {
      BufferUtil::hostAlloc(buf, rect.getWidth(), rect.getHeight(), buf->elem_chs);
      copy_from_device = true;
    }
  }
  if (copy_from_device) {
    BufferUtil::deviceToHostCopyEnqueue(buf, MemoryAccess::READ_WRITE);
    ExecutionManager::deviceWaitQueueToFinish();
  }

  return COM_Bmp::generateBitmapImage(rect, filename);
}

void PixelsUtil::saveAsImage(
    const unsigned char *img_buffer, int width, int height, int n_channels, std::string filename)
{
  return COM_Bmp::generateBitmapImage(img_buffer, width, height, n_channels, filename);
}

#endif