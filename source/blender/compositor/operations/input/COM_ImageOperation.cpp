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
#include "IMB_imbuf.h"
#include "IMB_imbuf_types.h"
#include "RE_pipeline.h"
#include "RE_render_ext.h"
#include "RE_shader_ext.h"

#include "COM_kernel_cpu.h"

BaseImageOperation::BaseImageOperation() : NodeOperation()
{
  this->m_image = NULL;
  this->m_buffer = NULL;
  this->m_imageFloatBuffer = NULL;
  this->m_imageByteBuffer = NULL;
  this->m_imageUser = NULL;
  this->m_imagewidth = 0;
  this->m_imageheight = 0;
  this->m_framenumber = 0;
  this->m_depthBuffer = NULL;
  this->m_numberOfChannels = 0;
  this->m_rd = NULL;
  this->m_viewName = NULL;
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

ImBuf *BaseImageOperation::getImBuf()
{
  ImBuf *ibuf;
  ImageUser iuser = *this->m_imageUser;

  if (this->m_image == NULL) {
    return NULL;
  }

  /* local changes to the original ImageUser */
  if (BKE_image_is_multilayer(this->m_image) == false) {
    iuser.multi_index = BKE_scene_multiview_view_id_get(this->m_rd, this->m_viewName);
  }

  ibuf = BKE_image_acquire_ibuf(this->m_image, &iuser, NULL);
  if (ibuf == NULL || (ibuf->rect == NULL && ibuf->rect_float == NULL)) {
    BKE_image_release_ibuf(this->m_image, ibuf, NULL);
    return NULL;
  }
  return ibuf;
}

void BaseImageOperation::initExecution()
{
  ImBuf *stackbuf = getImBuf();
  this->m_buffer = stackbuf;
  if (stackbuf) {
    this->m_imageFloatBuffer = stackbuf->rect_float;
    this->m_imageByteBuffer = stackbuf->rect;
    this->m_depthBuffer = stackbuf->zbuf_float;
    this->m_imagewidth = stackbuf->x;
    this->m_imageheight = stackbuf->y;
    this->m_numberOfChannels = stackbuf->channels;
  }
  NodeOperation::initExecution();
}

void BaseImageOperation::deinitExecution()
{
  this->m_imageFloatBuffer = NULL;
  this->m_imageByteBuffer = NULL;
  BKE_image_release_ibuf(this->m_image, this->m_buffer, NULL);
}

ResolutionType BaseImageOperation::determineResolution(int resolution[2],
                                                       int /*preferredResolution*/[2],
                                                       bool /*setResolution*/)
{
  ImBuf *stackbuf = getImBuf();

  resolution[0] = 0;
  resolution[1] = 0;

  if (stackbuf) {
    resolution[0] = stackbuf->x;
    resolution[1] = stackbuf->y;
  }

  BKE_image_release_ibuf(this->m_image, stackbuf, NULL);
  return ResolutionType::Fixed;
}

void BaseImageOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_framenumber);
  if (m_image) {
    hashParam(m_image->id.session_uuid);
  }
  else {
    hashParam((size_t)m_imageFloatBuffer);
    hashParam((size_t)m_imageByteBuffer);
  }
}

void ImageOperation::execPixels(ExecutionManager &man)
{
  float norm_mult = 1.0 / 255.0;
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    if (m_imageFloatBuffer == nullptr && m_imageByteBuffer == nullptr) {
      PixelsUtil::setRectElem(dst, (float *)&CCL::TRANSPARENT_PIXEL);
    }
    else if (m_imageFloatBuffer) {
      auto buf = BufferUtil::createUnmanagedTmpBuffer(
          m_imageFloatBuffer, m_imagewidth, m_imageheight, m_numberOfChannels, true);
      PixelsRect src_rect = PixelsRect(buf.get(), dst);
      PixelsUtil::copyEqualRectsNChannels(dst, src_rect, m_numberOfChannels);
    }
    else {
      int n_channels = m_numberOfChannels;
      size_t n_channels_sz = m_numberOfChannels;
      size_t width = m_width;
      unsigned char *uchar_buf = (unsigned char *)m_imageByteBuffer;
      CCL::float4 src_pixel;
      size_t src_offset;

      WRITE_DECL(dst);
      CPU_LOOP_START(dst);

      src_offset = n_channels == dst_img.elem_chs ?
                       dst_offset :
                       width * dst_coords.y * n_channels_sz + dst_coords.x * n_channels_sz;

      src_pixel = CCL::make_float4(uchar_buf[src_offset],
                                   uchar_buf[src_offset + 1],
                                   uchar_buf[src_offset + 2],
                                   uchar_buf[src_offset + 3]);
      // normalize
      src_pixel *= norm_mult;

      WRITE_IMG(dst, src_pixel);

      CPU_LOOP_END;

      IMB_colormanagement_colorspace_to_scene_linear(dst_img.start,
                                                     dst_img.row_elems,
                                                     dst_img.col_elems,
                                                     dst_img.elem_chs,
                                                     m_buffer->rect_colorspace,
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
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    if (m_imageFloatBuffer == nullptr && m_imageByteBuffer == nullptr) {
      PixelsUtil::setRectElem(dst, 0.0f);
    }
    else if (m_imageFloatBuffer) {
      auto buf = BufferUtil::createUnmanagedTmpBuffer(
          m_imageFloatBuffer, m_imagewidth, m_imageheight, COM_NUM_CHANNELS_VALUE, true);
      PixelsRect src_rect = PixelsRect(buf.get(), dst);
      PixelsUtil::copyEqualRectsNChannels(dst, src_rect, COM_NUM_CHANNELS_VALUE);
    }
    else {
      unsigned char *uchar_buf = (unsigned char *)m_imageByteBuffer;
      WRITE_DECL(dst);
      CPU_LOOP_START(dst);

      int src_offset = m_width * dst_coords.y * m_numberOfChannels +
                       dst_coords.x * m_numberOfChannels;

      // normalize to float
      dst_img.buffer[dst_offset] = uchar_buf[src_offset + 3] / 255.0f;

      CPU_LOOP_END;
    }
  };
  return cpuWriteSeek(man, cpuWrite);
}

void ImageAlphaOperation::hashParams()
{
  BaseImageOperation::hashParams();
}

void ImageDepthOperation::execPixels(ExecutionManager &man)
{
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    if (m_depthBuffer == NULL) {
      PixelsUtil::setRectElem(dst, 0.0f);
    }
    else {
      auto buf = BufferUtil::createUnmanagedTmpBuffer(
          m_depthBuffer, m_imagewidth, m_imageheight, COM_NUM_CHANNELS_VALUE, true);
      PixelsRect src_rect = PixelsRect(buf.get(), dst);
      PixelsUtil::copyEqualRectsNChannels(dst, src_rect, 1);
    }
  };
  cpuWriteSeek(man, cpuWrite);
}

void ImageDepthOperation::hashParams()
{
  BaseImageOperation::hashParams();
  hashParam((size_t)m_depthBuffer);
}