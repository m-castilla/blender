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
#include "COM_kernel_cpu.h"
#include "IMB_imbuf_types.h"
#include <string.h>

int PixelsUtil::getNUsedChannels(DataType dataType)
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
      return 0;
  }
}

DataType PixelsUtil::getNUsedChannelsDataType(int n_used_chs)
{
  switch (n_used_chs) {
    case COM_NUM_CHANNELS_VALUE:
      return DataType::VALUE;
    case COM_NUM_CHANNELS_VECTOR:
      return DataType::VECTOR;
    case COM_NUM_CHANNELS_COLOR:
      return DataType::COLOR;
    default:
      BLI_assert(!"There is no implemented DataType for this number of channels");
      return (DataType)0;
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

void PixelsUtil::copyBufferRect(PixelsRect &dst, float *read_buf, int n_used_chs, int n_buf_chs)
{
  auto buf = BufferUtil::createNonStdTmpBuffer(
      read_buf, true, dst.tmp_buffer->width, dst.tmp_buffer->height, n_buf_chs);
  PixelsRect src_rect = PixelsRect(buf.get(), dst);
  PixelsUtil::copyEqualRectsNChannels(dst, src_rect, n_used_chs);
}

void PixelsUtil::copyBufferRectNChannels(PixelsRect &dst,
                                         float *read_buf,
                                         int n_channels,
                                         int n_buf_chs)
{
  copyBufferRect(dst, read_buf, n_channels, n_buf_chs);
}

void PixelsUtil::copyBufferRectChannel(
    PixelsRect &dst, int to_ch_idx, float *read_buf, int from_ch_idx, int n_buf_chs)
{
  auto buf = BufferUtil::createNonStdTmpBuffer(
      read_buf, true, dst.tmp_buffer->width, dst.tmp_buffer->height, n_buf_chs);
  PixelsRect src_rect = PixelsRect(buf.get(), dst);
  PixelsUtil::copyEqualRectsChannel(dst, to_ch_idx, src_rect, from_ch_idx);
}

void PixelsUtil::copyBufferRect(PixelsRect &dst,
                                unsigned char *read_buf,
                                int n_used_chs,
                                int n_buf_chs)
{
  float norm_mult = 1.0 / 255.0;
  CCL::float4 src_pixel;
  size_t src_offset = dst.ymin * dst.tmp_buffer->width * n_buf_chs + dst.xmin * n_buf_chs;
  WRITE_DECL(dst);

  if (n_used_chs == 4) {
    CPU_LOOP_START(dst);
    src_pixel = CCL::make_float4(read_buf[src_offset],
                                 read_buf[src_offset + 1],
                                 read_buf[src_offset + 2],
                                 read_buf[src_offset + 3]);
    src_pixel *= norm_mult;  // normalize
    src_offset += n_buf_chs;
    WRITE_IMG(dst, src_pixel);
    CPU_LOOP_END;
  }
  else if (n_used_chs == 3) {
    CPU_LOOP_START(dst);
    src_pixel.x = read_buf[src_offset];
    src_pixel.y = read_buf[src_offset];
    src_pixel.z = read_buf[src_offset];
    src_pixel *= norm_mult;  // normalize
    src_offset += n_buf_chs;
    WRITE_IMG(dst, src_pixel);
    CPU_LOOP_END;
  }
  else {
    CPU_LOOP_START(dst);
    src_pixel.x = read_buf[src_offset] * norm_mult;  // normalize
    src_offset += n_buf_chs;
    WRITE_IMG(dst, src_pixel);
    CPU_LOOP_END;
  }
}

void PixelsUtil::copyBufferRectNChannels(PixelsRect &dst,
                                         unsigned char *read_buf,
                                         int n_channels,
                                         int n_buf_chs)
{
  copyBufferRect(dst, read_buf, n_channels, n_buf_chs);
}

void PixelsUtil::copyBufferRectChannel(
    PixelsRect &dst, int to_ch_idx, unsigned char *read_buf, int from_ch_idx, int n_buf_chs)
{
  float norm_mult = 1.0 / 255.0;
  PixelsImg dst_img = dst.pixelsImg();
  float *dst_cur = dst_img.start + to_ch_idx;

  size_t buf_brow_chs = dst.tmp_buffer->width * n_buf_chs;
  unsigned char *buf_cur = read_buf + dst.ymin * buf_brow_chs + dst.xmin * n_buf_chs + from_ch_idx;

  float *dst_row_end = dst_img.start + dst_img.row_chs;
  while (dst_cur < dst_img.end) {
    while (dst_cur < dst_row_end) {
      *dst_cur = *buf_cur * norm_mult;

      dst_cur += dst_img.belem_chs;
      buf_cur += n_buf_chs;
    }
    dst_cur += dst_img.row_jump;
    dst_row_end += dst_img.brow_chs_incr;
  }
}

bool PixelsUtil::copyImBufRect(PixelsRect &dst, ImBuf *imbuf, int n_used_chs, int n_buf_chs)
{
  if (!BufferUtil::isImBufAvailable(imbuf)) {
    PixelsUtil::setRectElem(dst, (float *)&CCL::TRANSPARENT_PIXEL);
    return false;
  }
  else if (imbuf->rect_float) {
    copyBufferRect(dst, imbuf->rect_float, n_used_chs, n_buf_chs);
    return false;
  }
  else {
    copyBufferRect(dst, (unsigned char *)imbuf->rect, n_used_chs, n_buf_chs);
    return true;
  }
}
bool PixelsUtil::copyImBufRectNChannels(PixelsRect &dst,
                                        ImBuf *imbuf,
                                        int n_channels,
                                        int n_buf_chs)
{
  return copyImBufRect(dst, imbuf, n_channels, n_buf_chs);
}
bool PixelsUtil::copyImBufRectChannel(
    PixelsRect &dst, int to_ch_idx, ImBuf *imbuf, int from_ch_idx, int n_buf_chs)
{
  if (!BufferUtil::isImBufAvailable(imbuf)) {
    PixelsUtil::setRectElem(dst, (float *)&CCL::TRANSPARENT_PIXEL);
    return false;
  }
  else if (imbuf->rect_float) {
    copyBufferRectChannel(dst, to_ch_idx, imbuf->rect_float, from_ch_idx, n_buf_chs);
    return false;
  }
  else {
    copyBufferRectChannel(dst, to_ch_idx, (unsigned char *)imbuf->rect, from_ch_idx, n_buf_chs);
    return true;
  }
}

void PixelsUtil::copyBufferRect(
    float *dst_buf, PixelsRect &dst_rect, int n_used_chs, int n_buf_chs, PixelsRect &src_rect)
{
  auto buf = BufferUtil::createNonStdTmpBuffer(
      dst_buf, true, dst_rect.getWidth(), dst_rect.getHeight(), n_buf_chs);
  PixelsRect dst_buf_rect = PixelsRect(buf.get(), dst_rect);
  PixelsUtil::copyEqualRectsNChannels(dst_buf_rect, src_rect, n_used_chs);
}

void PixelsUtil::copyBufferRectNChannels(
    float *dst_buf, PixelsRect &dst_rect, int n_used_chs, int n_channels, PixelsRect &src_rect)
{
  copyBufferRect(dst_buf, dst_rect, n_used_chs, n_channels, src_rect);
}

void PixelsUtil::copyBufferRectChannel(float *dst_buf,
                                       int to_ch_idx,
                                       PixelsRect &dst_rect,
                                       int n_buf_chs,
                                       PixelsRect &src_rect,
                                       int from_ch_idx)
{
  auto buf = BufferUtil::createNonStdTmpBuffer(
      dst_buf, true, dst_rect.getWidth(), dst_rect.getHeight(), n_buf_chs);
  PixelsRect dst_buf_rect = PixelsRect(buf.get(), dst_rect);
  PixelsUtil::copyEqualRectsChannel(dst_buf_rect, to_ch_idx, src_rect, from_ch_idx);
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

    BLI_assert(w1.row_jump >= 0 && r1.row_jump >= 0);
    if (w1.row_bytes == r1.row_bytes) {
      if (w1.row_jump == 0 && r1.row_jump == 0) {
        memcpy(w1.start, r1.start, (w1.end - w1.start) * sizeof(float));
      }
      else {
        float *w1_cur = w1.start;
        float *r1_cur = r1.start;
        while (w1_cur < w1.end) {
          memcpy(w1_cur, r1_cur, r1.row_bytes);
          w1_cur += w1.brow_chs_incr;
          r1_cur += r1.brow_chs_incr;
        }
      }
    }
    else {
      BLI_assert(w1.elem_bytes == r1.elem_bytes);
      float *w1_cur = w1.start;
      float *r1_cur = r1.start;
      float *w1_row_end = w1.start + w1.row_chs;
      while (w1_cur < w1.end) {
        while (w1_cur < w1_row_end) {
          memcpy(w1_cur, r1_cur, w1.elem_bytes);
          w1_cur += w1.belem_chs_incr;
          r1_cur += r1.belem_chs_incr;
        }
        w1_cur += w1.row_jump;
        r1_cur += r1.row_jump;
        w1_row_end += w1.brow_chs_incr;
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
  else if (wr1.tmp_buffer->elem_chs3 == rr1.tmp_buffer->elem_chs3 &&
           wr1.tmp_buffer->elem_chs3 == n_channels) {
    copyEqualRects(wr1, rr1);
  }
  else {
    PixelsImg w1 = wr1.pixelsImg();
    PixelsImg r1 = rr1.pixelsImg();
    size_t used_elem_bytes = n_channels * sizeof(float);
    BLI_assert(w1.elem_bytes >= used_elem_bytes && r1.elem_bytes >= used_elem_bytes);
    float *w1_cur = w1.start;
    float *r1_cur = r1.start;
    float *w1_row_end = w1.start + w1.row_chs;
    while (w1_cur < w1.end) {
      while (w1_cur < w1_row_end) {
        memcpy(w1_cur, r1_cur, used_elem_bytes);

        w1_cur += w1.belem_chs;
        r1_cur += r1.belem_chs;
      }
      w1_cur += w1.row_jump;
      r1_cur += r1.row_jump;
      w1_row_end += w1.brow_chs_incr;
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
    BLI_assert(w1.row_elems == c1.row_elems && w1.elem_chs3 > wr_channel &&
               c1.elem_chs3 > cr_channel);
    float *w1_cur = w1.start + wr_channel;
    float *c1_cur = c1.start + cr_channel;
    float *w1_row_end = w1.start + w1.row_chs;
    while (w1_cur < w1.end) {
      while (w1_cur < w1_row_end) {
        *w1_cur = *c1_cur;

        w1_cur += w1.belem_chs;
        c1_cur += c1.belem_chs;
      }
      w1_cur += w1.row_jump;
      w1_row_end += w1.brow_chs_incr;
      c1_cur += c1.row_jump;
    }
  }
}

void PixelsUtil::setRectChannel(PixelsRect &wr1, int channel, float channel_value)
{
  // write rect should never be single element
  BLI_assert(!wr1.is_single_elem);
  PixelsImg w1 = wr1.pixelsImg();
  BLI_assert(w1.elem_chs3 > channel);
  float *w1_cur = w1.start + channel;
  float *w1_row_end = w1.start + w1.row_chs;
  while (w1_cur < w1.end) {
    while (w1_cur < w1_row_end) {
      *w1_cur = channel_value;

      w1_cur += w1.belem_chs;
    }
    w1_cur += w1.row_jump;
    w1_row_end += w1.brow_chs_incr;
  }
}

void PixelsUtil::setRectElem(PixelsRect &wr1, const float *elem)
{
  setRectElem(wr1, elem, wr1.tmp_buffer->elem_chs3);
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
  BLI_assert(w1.elem_chs3 >= n_channels);
  float *w1_cur = w1.start;
  float *w1_row_end = w1.start + w1.row_chs;
  size_t chs_bytes = n_channels * sizeof(float);
  while (w1_cur < w1.end) {
    while (w1_cur < w1_row_end) {
      memcpy(w1_cur, elem, chs_bytes);
      w1_cur += w1.belem_chs;
    }
    w1_cur += w1.row_jump;
    w1_row_end += w1.brow_chs_incr;
  }
}

#if defined(COM_DEBUG) || defined(DEBUG)

void PixelsUtil::saveAsImage(TmpBuffer *buf, std::string filename)
{
  PixelsRect rect(buf, 0, buf->width, 0, buf->height);
  saveAsImage(rect, filename);
}

void PixelsUtil::saveAsImage(PixelsRect &rect, std::string filename)
{
  auto buf = rect.tmp_buffer;
  bool copy_from_device = false;
  if (buf->device.state == DeviceMemoryState::FILLED) {
    if (buf->host.state == HostMemoryState::CLEARED) {
      copy_from_device = true;
    }
    else if (buf->host.state == HostMemoryState::NONE) {
      BufferUtil::hostNonStdAlloc(buf, rect.getWidth(), rect.getHeight(), buf->host.belem_chs3);
      copy_from_device = true;
    }
  }
  if (copy_from_device) {
    BufferUtil::deviceToHostCopyEnqueue(buf);
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