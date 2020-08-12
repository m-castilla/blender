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

#include "COM_MultilayerImageOperation.h"

#include "COM_BufferUtil.h"
#include "COM_PixelsUtil.h"
#include "COM_kernel_cpu_nocompat.h"
#include "DNA_image_types.h"
#include "IMB_imbuf.h"
#include "IMB_imbuf_types.h"

MultilayerBaseOperation::MultilayerBaseOperation(int passindex, int view) : BaseImageOperation()
{
  this->m_passId = passindex;
  this->m_view = view;
}

ImBuf *MultilayerBaseOperation::getImBuf()
{
  /* temporarily changes the view to get the right ImBuf */
  int view = this->m_imageUser->view;

  this->m_imageUser->view = this->m_view;
  this->m_imageUser->pass = this->m_passId;

  if (BKE_image_multilayer_index(this->m_image->rr, this->m_imageUser)) {
    ImBuf *ibuf = BaseImageOperation::getImBuf();
    this->m_imageUser->view = view;
    return ibuf;
  }

  this->m_imageUser->view = view;
  return NULL;
}

void MultilayerBaseOperation::hashParams()
{
  BaseImageOperation::hashParams();
  hashParam(this->m_view);
  hashParam(this->m_passId);
}

void MultilayerColorOperation::execPixels(ExecutionManager &man)
{
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    if (m_imageFloatBuffer == NULL) {
      PixelsUtil::setRectElem(dst, (float *)&CCL_NAMESPACE::TRANSPARENT_PIXEL);
    }
    else {
      auto buf = BufferUtil::createUnmanagedTmpBuffer(
          m_imageFloatBuffer, m_imagewidth, m_imageheight, m_numberOfChannels, true);
      PixelsRect src_rect = PixelsRect(buf.get(), dst);
      PixelsUtil::copyEqualRectsNChannels(src_rect, dst, m_numberOfChannels);
    }
  };
  return cpuWriteSeek(man, cpuWrite);
}

void MultilayerValueOperation::execPixels(ExecutionManager &man)
{
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    if (m_imageFloatBuffer == NULL) {
      PixelsUtil::setRectElem(dst, (float *)&CCL_NAMESPACE::TRANSPARENT_PIXEL);
    }
    else {
      auto buf = BufferUtil::createUnmanagedTmpBuffer(
          m_imageFloatBuffer, m_imagewidth, m_imageheight, m_numberOfChannels, true);
      PixelsRect src_rect = PixelsRect(buf.get(), dst);
      PixelsUtil::copyEqualRectsNChannels(src_rect, dst, COM_NUM_CHANNELS_VALUE);
    }
  };
  return cpuWriteSeek(man, cpuWrite);
}

void MultilayerVectorOperation::execPixels(ExecutionManager &man)
{
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    if (m_imageFloatBuffer == NULL) {
      PixelsUtil::setRectElem(dst, (float *)&CCL_NAMESPACE::TRANSPARENT_PIXEL);
    }
    else {
      auto buf = BufferUtil::createUnmanagedTmpBuffer(
          m_imageFloatBuffer, m_imagewidth, m_imageheight, m_numberOfChannels, true);
      PixelsRect src_rect = PixelsRect(buf.get(), dst);
      PixelsUtil::copyEqualRectsNChannels(src_rect, dst, COM_NUM_CHANNELS_VECTOR);
    }
  };
  return cpuWriteSeek(man, cpuWrite);
}
