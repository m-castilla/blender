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
#include "COM_BufferUtil.h"
#include "COM_MovieClipOperation.h"
#include "COM_PixelsUtil.h"
#include "IMB_imbuf.h"

#include "COM_kernel_cpu.h"

using namespace std::placeholders;

MovieClipBaseOperation::MovieClipBaseOperation() : NodeOperation()
{
  this->m_movieClip = NULL;
  this->m_clip_imbuf = NULL;
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

    this->m_clip_imbuf = ibuf;
    // if (ibuf) {
    //  this->m_clip_imbuf = ibuf;
    //  if (ibuf->rect_float == NULL || ibuf->userflags & IB_RECT_INVALID) {
    //    IMB_float_from_rect(ibuf);
    //    ibuf->userflags &= ~IB_RECT_INVALID;
    //  }
    //}
  }
  NodeOperation::initExecution();
}

void MovieClipBaseOperation::deinitExecution()
{
  if (this->m_clip_imbuf) {
    IMB_freeImBuf(this->m_clip_imbuf);

    this->m_clip_imbuf = NULL;
  }
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
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    // on initExecution float buffer is assured, so no color space conversion needed
    int n_channels = m_clip_imbuf->channels == 0 ? COM_NUM_CHANNELS_COLOR : m_clip_imbuf->channels;
    PixelsUtil::copyImBufRect(dst, m_clip_imbuf, n_channels, n_channels);
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
    // on initExecution float buffer is assured, so no color space conversion needed
    int n_channels = m_clip_imbuf->channels == 0 ? COM_NUM_CHANNELS_COLOR : m_clip_imbuf->channels;
    PixelsUtil::copyImBufRectChannel(dst, 0, m_clip_imbuf, 3, n_channels);
  };
  cpuWriteSeek(man, cpuWrite);
}
