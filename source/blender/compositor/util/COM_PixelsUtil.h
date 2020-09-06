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

#ifndef __COM_PIXELSUTIL_H__
#define __COM_PIXELSUTIL_H__

#include "COM_Rect.h"

class NodeOperation;
class ExecutionManager;
struct ImBuf;
namespace PixelsUtil {
int getNUsedChannels(DataType dataType);
DataType getNUsedChannelsDataType(int n_used_chs);
SocketType dataToSocketType(DataType d);

void copyBufferRect(PixelsRect &dst, float *read_buf, int n_used_chs, int n_buf_chs);
void copyBufferRectNChannels(PixelsRect &dst, float *read_buf, int n_channels, int n_buf_chs);
void copyBufferRectChannel(
    PixelsRect &dst, int to_ch_idx, float *read_buf, int from_ch_idx, int n_buf_chs);

void copyBufferRect(PixelsRect &dst, unsigned char *read_buf, int n_used_chs, int n_buf_chs);
void copyBufferRectNChannels(PixelsRect &dst,
                             unsigned char *read_buf,
                             int n_channels,
                             int n_buf_chs);
void copyBufferRectChannel(
    PixelsRect &dst, int to_ch_idx, unsigned char *read_buf, int from_ch_idx, int n_buf_chs);

// returns whether the ImgBuf byte buffer was used for reading or not
bool copyImBufRect(PixelsRect &dst, ImBuf *imbuf, int n_used_chs, int n_buf_chs);
// returns whether the ImgBuf byte buffer was used for reading or not
bool copyImBufRectNChannels(PixelsRect &dst, ImBuf *imbuf, int n_channels, int n_buf_chs);
// returns whether the ImgBuf byte buffer was used for reading or not
bool copyImBufRectChannel(
    PixelsRect &dst, int to_ch_idx, ImBuf *imbuf, int from_ch_idx, int n_buf_chs);

void copyBufferRect(
    float *dst_buf, PixelsRect &dst_rect, int n_used_chs, int n_buf_chs, PixelsRect &src_rect);
void copyBufferRectNChannels(
    float *dst_buf, PixelsRect &dst_rect, int n_used_chs, int n_channels, PixelsRect &src_rect);
void copyBufferRectChannel(float *dst_buf,
                           int to_ch_idx,
                           PixelsRect &dst_rect,
                           int n_buf_chs,
                           PixelsRect &src_rect,
                           int from_ch_idx);

void copyEqualRects(PixelsRect &wr1, PixelsRect &rr1);
void copyEqualRectsNChannels(PixelsRect &wr1, PixelsRect &rr1, int n_channels);
void copyEqualRectsChannel(PixelsRect &wr1, int wr_channel, PixelsRect &cr1, int cr_channel);
void setRectChannel(PixelsRect &wr1, int channel, float channel_value);
void setRectElem(PixelsRect &wr1, const float *elem);
void setRectElem(PixelsRect &wr1, const float elem);
void setRectElem(PixelsRect &wr1, const float *elem, int n_channels);

#if defined(COM_DEBUG) || defined(DEBUG)
void saveAsImage(TmpBuffer *buf, std::string filename);
void saveAsImage(PixelsRect &rect, std::string filename);
void saveAsImage(
    const unsigned char *img_buffer, int width, int height, int n_channels, std::string filename);
#endif

};  // namespace PixelsUtil

#endif
