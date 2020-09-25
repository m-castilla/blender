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

#include "COM_ImageOperation.h"
#include "BKE_image.h"
#include "BKE_scene.h"
#include "COM_BufferUtil.h"
#include "COM_PixelsUtil.h"
#include "DNA_image_types.h"
#include "IMB_colormanagement.h"
#include "IMB_imbuf_types.h"
#include "RE_pipeline.h"
#include "RE_render_ext.h"
#include "RE_shader_ext.h"

#include "COM_kernel_cpu.h"

BaseImageOperation::BaseImageOperation() : NodeOperation()
{
  this->m_image = NULL;
  this->m_imbuf = NULL;
  this->m_imageUser = NULL;
  this->m_framenumber = 0;
  this->m_rd = NULL;
  this->m_viewName = NULL;
  im_buf_requested = false;
}
ImageOperation::ImageOperation() : BaseImageOperation()
{
  this->addOutputSocket(SocketType::COLOR);
}
ImageAlphaOperation::ImageAlphaOperation() : BaseImageOperation()
{
  this->addOutputSocket(SocketType::VALUE);
}
ImageDepthOperation::ImageDepthOperation() : BaseImageOperation()
{
  this->addOutputSocket(SocketType::VALUE);
}

void BaseImageOperation::requestImBuf()
{
  if (!im_buf_requested) {
    im_buf_requested = true;
    ImageUser iuser = *this->m_imageUser;
    if (this->m_image == NULL) {
      return;
    }

    /* local changes to the original ImageUser */
    if (BKE_image_is_multilayer(this->m_image) == false) {
      iuser.multi_index = BKE_scene_multiview_view_id_get(this->m_rd, this->m_viewName);
    }

    m_imbuf = BKE_image_acquire_ibuf(this->m_image, &iuser, NULL);
    if (!BufferUtil::isImBufAvailable(m_imbuf)) {
      BKE_image_release_ibuf(this->m_image, m_imbuf, NULL);
    }
  }
}

void BaseImageOperation::initExecution()
{
  requestImBuf();
  NodeOperation::initExecution();
}

void BaseImageOperation::deinitExecution()
{
  BKE_image_release_ibuf(this->m_image, this->m_imbuf, NULL);
  NodeOperation::deinitExecution();
}

ResolutionType BaseImageOperation::determineResolution(int resolution[2],
                                                       int /*preferredResolution*/[2],
                                                       bool /*setResolution*/)
{
  requestImBuf();

  resolution[0] = 0;
  resolution[1] = 0;

  if (BufferUtil::isImBufAvailable(m_imbuf)) {
    resolution[0] = m_imbuf->x;
    resolution[1] = m_imbuf->y;
  }

  return ResolutionType::Fixed;
}

void BaseImageOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_framenumber);
  if (m_image) {
    hashParam(m_image->id.session_uuid);
  }
  else if (BufferUtil::isImBufAvailable(m_imbuf)) {
    hashParam((size_t)m_imbuf->rect_float);
    hashParam((size_t)m_imbuf->rect);
  }
}

void ImageOperation::execPixels(ExecutionManager &man)
{
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    bool byte_buf_used = PixelsUtil::copyImBufRect(
        dst, m_imbuf, COM_NUM_CHANNELS_COLOR, COM_NUM_CHANNELS_COLOR);
    if (byte_buf_used) {
      PixelsImg dst_img = dst.pixelsImg();
      IMB_colormanagement_colorspace_to_scene_linear(dst_img.start,
                                                     dst_img.row_elems,
                                                     dst_img.col_elems,
                                                     dst_img.belem_chs,
                                                     m_imbuf->rect_colorspace,
                                                     false);
    }
  };

  cpuWriteSeek(man, cpuWrite);
}

void ImageOperation::hashParams()
{
  BaseImageOperation::hashParams();
}

void ImageAlphaOperation::execPixels(ExecutionManager &man)
{
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext &) {
    PixelsUtil::copyImBufRectChannel(dst, 0, m_imbuf, 3, COM_NUM_CHANNELS_COLOR);
  };
  return cpuWriteSeek(man, cpuWrite);
}

void ImageAlphaOperation::hashParams()
{
  BaseImageOperation::hashParams();
}

void ImageDepthOperation::execPixels(ExecutionManager &man)
{
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext &) {
    if (m_imbuf->zbuf_float == nullptr) {
      PixelsUtil::setRectElem(dst, 0.0f);
    }
    else {
      PixelsUtil::copyBufferRect(dst, m_imbuf->zbuf_float, 1, 1);
    }
  };
  cpuWriteSeek(man, cpuWrite);
}

void ImageDepthOperation::hashParams()
{
  BaseImageOperation::hashParams();
  hashParam((size_t)m_imbuf->zbuf_float);
}