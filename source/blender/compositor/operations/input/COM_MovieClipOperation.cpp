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

#include <cstring>

#include "BKE_image.h"
#include "BKE_movieclip.h"
#include "BLI_listbase.h"
#include "IMB_colormanagement.h"
#include "IMB_imbuf.h"

#include "COM_BufferUtil.h"
#include "COM_ExecutionManager.h"
#include "COM_MovieClipOperation.h"
#include "COM_PixelsUtil.h"

#include "COM_kernel_cpu.h"

using namespace std::placeholders;

MovieClipBaseOperation::MovieClipBaseOperation() : NodeOperation()
{
  this->m_movieClip = NULL;
  this->m_clip_imbuf = NULL;
  this->m_movieClipUser = NULL;
  this->m_framenumber = 0;
  this->m_cacheFrame = false;
}

void MovieClipBaseOperation::initExecution()
{
  if (this->m_movieClip) {
    BKE_movieclip_user_set_frame(this->m_movieClipUser, this->m_framenumber);

    if (this->m_cacheFrame) {
      m_clip_imbuf = BKE_movieclip_get_ibuf(this->m_movieClip, this->m_movieClipUser);
    }
    else {
      m_clip_imbuf = BKE_movieclip_get_ibuf_flag(
          this->m_movieClip, this->m_movieClipUser, this->m_movieClip->flag, MOVIECLIP_CACHE_SKIP);
    }
  }
  NodeOperation::initExecution();
}

void MovieClipBaseOperation::deinitExecution()
{
  if (this->m_clip_imbuf) {
    IMB_freeImBuf(this->m_clip_imbuf);

    this->m_clip_imbuf = NULL;
  }
  NodeOperation::deinitExecution();
}

ResolutionType MovieClipBaseOperation::determineResolution(int resolution[2],
                                                           int /*preferredResolution*/[2],

                                                           bool /*setResolution*/)
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
    if (BufferUtil::isImBufAvailable(m_clip_imbuf)) {
      hashParam((size_t)m_clip_imbuf->rect);
      hashParam((size_t)m_clip_imbuf->rect_float);
      hashParam(std::string(m_clip_imbuf->name));
      hashParam(std::string(m_clip_imbuf->cachename));
    }
  }
}

void MovieClipOperation::execPixels(ExecutionManager &man)
{
  m_n_written_rects = 0;
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    if (BufferUtil::isImBufAvailable(m_clip_imbuf)) {
      int n_channels = m_clip_imbuf->channels == 0 ? COM_NUM_CHANNELS_COLOR :
                                                     m_clip_imbuf->channels;
      BLI_assert(n_channels == COM_NUM_CHANNELS_COLOR);
      PixelsUtil::copyImBufRect(dst, m_clip_imbuf, n_channels, n_channels);

      // if buffer width is the same as rect width, there is no pitch, so we may use the colorspace
      // method by rect, otherwise it has to be the whole operation rect because it doesn't support
      // pitch
      if (dst.getWidth() == dst.tmp_buffer->host.bwidth) {
        PixelsImg img = dst.pixelsImg();
        IMB_colormanagement_colorspace_to_scene_linear(img.start,
                                                       img.row_elems,
                                                       img.col_elems,
                                                       n_channels,
                                                       m_clip_imbuf->rect_colorspace,
                                                       false);
      }
      else {
        m_mutex.lock();
        m_n_written_rects++;
        if (m_n_written_rects == ctx.n_rects) {
          IMB_colormanagement_colorspace_to_scene_linear(dst.tmp_buffer->host.buffer,
                                                         dst.tmp_buffer->width,
                                                         dst.tmp_buffer->height,
                                                         n_channels,
                                                         m_clip_imbuf->rect_colorspace,
                                                         false);
        }
        m_mutex.unlock();
      }
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
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    int n_channels = m_clip_imbuf->channels == 0 ? COM_NUM_CHANNELS_COLOR : m_clip_imbuf->channels;
    BLI_assert(n_channels == COM_NUM_CHANNELS_COLOR);
    PixelsUtil::copyImBufRectChannel(dst, 0, m_clip_imbuf, 3, n_channels);
  };
  cpuWriteSeek(man, cpuWrite);
}
