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

#include "COM_MovieClipOperation.h"

#include "BLI_listbase.h"

#include "BKE_image.h"
#include "BKE_movieclip.h"

#include "COM_BufferUtil.h"
#include "COM_PixelsUtil.h"
#include "COM_kernel_cpu_nocompat.h"
#include "IMB_imbuf.h"

using namespace std::placeholders;

MovieClipBaseOperation::MovieClipBaseOperation() : NodeOperation()
{
  this->m_movieClip = NULL;
  this->m_movieClipBuffer = NULL;
  this->m_movieClipUser = NULL;
  this->m_framenumber = 0;
  this->m_cacheFrame = 0;
}

void MovieClipBaseOperation::initExecution()
{
  if (this->m_movieClip) {
    BKE_movieclip_user_set_frame(this->m_movieClipUser, this->m_framenumber);
    ImBuf *ibuf;

    if (this->m_cacheFrame) {
      ibuf = BKE_movieclip_get_ibuf(this->m_movieClip, this->m_movieClipUser);
    }
    else {
      ibuf = BKE_movieclip_get_ibuf_flag(
          this->m_movieClip, this->m_movieClipUser, this->m_movieClip->flag, MOVIECLIP_CACHE_SKIP);
    }

    if (ibuf) {
      this->m_movieClipBuffer = ibuf;
      if (ibuf->rect_float == NULL || ibuf->userflags & IB_RECT_INVALID) {
        IMB_float_from_rect(ibuf);
        ibuf->userflags &= ~IB_RECT_INVALID;
      }
    }
  }
}

void MovieClipBaseOperation::deinitExecution()
{
  if (this->m_movieClipBuffer) {
    IMB_freeImBuf(this->m_movieClipBuffer);

    this->m_movieClipBuffer = NULL;
  }
}

ResolutionType MovieClipBaseOperation::determineResolution(int resolution[2],
                                                           int /*preferredResolution*/[2],

                                                           bool setResolution)
{
  resolution[0] = 0;
  resolution[1] = 0;

  if (this->m_movieClip) {
    int width, height;

    BKE_movieclip_get_size(this->m_movieClip, this->m_movieClipUser, &width, &height);

    resolution[0] = width;
    resolution[1] = height;
  }
  return ResolutionType::Fixed;
}

void MovieClipBaseOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_framenumber);
  hashParam(m_cacheFrame);
  if (m_movieClipUser) {
    hashParam(m_movieClipUser->framenr);
  }

  if (m_movieClip) {
    hashParam(m_movieClip->id.session_uuid);
  }
  else {
    if (m_movieClipBuffer) {
      hashParam((size_t)m_movieClipBuffer->rect);
      hashParam((size_t)m_movieClipBuffer->rect_float);
      hashParam(std::string(m_movieClipBuffer->name));
      hashParam(std::string(m_movieClipBuffer->cachename));
    }
  }
}

void MovieClipOperation::execPixels(ExecutionManager &man)
{
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    if (m_movieClipBuffer == nullptr ||
        (m_movieClipBuffer->rect == nullptr && m_movieClipBuffer->rect_float == nullptr)) {
      PixelsUtil::setRectElem(dst, (float *)&CCL_NAMESPACE::TRANSPARENT_PIXEL);
    }
    else if (m_movieClipBuffer->rect_float) {
      int n_channels = m_movieClipBuffer->channels == 0 ? 4 : m_movieClipBuffer->channels;
      auto buf = BufferUtil::createUnmanagedTmpBuffer(
          m_movieClipBuffer->rect_float, m_width, m_height, n_channels, true);
      PixelsRect src_rect = PixelsRect(buf.get(), dst);
      PixelsUtil::copyEqualRectsNChannels(dst, src_rect, n_channels);
    }
    else {
      int n_channels = m_movieClipBuffer->channels == 0 ? 4 : m_movieClipBuffer->channels;
      unsigned char *uchar_buf = (unsigned char *)m_movieClipBuffer->rect;

      CPU_WRITE_DECL(dst);
      CPU_LOOP_START(dst);

      CPU_WRITE_OFFSET(dst);
      int src_offset = m_width * (dst_start_y + write_offset_y) * n_channels +
                       (dst_start_x + write_offset_x) * n_channels;

      CCL_NAMESPACE::float4 src_pixel = CCL_NAMESPACE::make_float4(uchar_buf[src_offset],
                                                                   uchar_buf[src_offset + 1],
                                                                   uchar_buf[src_offset + 2],
                                                                   uchar_buf[src_offset + 3]);
      // normalize
      src_pixel /= 255.0f;
      memcpy(&dst_img.buffer[dst_offset], &src_pixel, sizeof(float) * 4);

      CPU_LOOP_END;
    }
  };
  cpuWriteSeek(man, cpuWrite);
}

MovieClipOperation::MovieClipOperation() : MovieClipBaseOperation()
{
  this->addOutputSocket(SocketType::COLOR);
}

MovieClipAlphaOperation::MovieClipAlphaOperation() : MovieClipBaseOperation()
{
  this->addOutputSocket(SocketType::VALUE);
}

void MovieClipAlphaOperation::execPixels(ExecutionManager &man)
{
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    if (m_movieClipBuffer == nullptr ||
        (m_movieClipBuffer->rect == nullptr && m_movieClipBuffer->rect_float == nullptr)) {
      PixelsUtil::setRectElem(dst, 0.0f);
    }
    else if (m_movieClipBuffer->rect_float) {
      int n_channels = m_movieClipBuffer->channels == 0 ? 4 : m_movieClipBuffer->channels;
      auto buf = BufferUtil::createUnmanagedTmpBuffer(
          m_movieClipBuffer->rect_float, m_width, m_height, n_channels, true);
      PixelsRect src_rect = PixelsRect(buf.get(), dst);
      PixelsUtil::copyEqualRectsChannel(dst, 0, src_rect, 3);
    }
    else {
      int n_channels = m_movieClipBuffer->channels == 0 ? 4 : m_movieClipBuffer->channels;
      unsigned char *uchar_buf = (unsigned char *)m_movieClipBuffer->rect;

      CPU_WRITE_DECL(dst);
      CPU_LOOP_START(dst);

      CPU_WRITE_OFFSET(dst);
      int src_offset = m_width * (dst_start_y + write_offset_y) * n_channels +
                       (dst_start_x + write_offset_x) * n_channels;

      // normalize to float
      dst_img.buffer[dst_offset] = uchar_buf[src_offset + 3] / 255.0f;

      CPU_LOOP_END;
    }
  };
  cpuWriteSeek(man, cpuWrite);
}
