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
#include "BLI_listbase.h"
#include "BLI_math.h"
#include "DNA_image_types.h"

#include "IMB_colormanagement.h"
#include "IMB_imbuf.h"
#include "IMB_imbuf_types.h"

#include "RE_pipeline.h"
#include "RE_texture.h"

namespace blender::compositor {

BaseImageOperation::BaseImageOperation()
{
  this->m_image = nullptr;
  this->m_buffer = nullptr;
  this->m_imageFloatBuffer = nullptr;
  this->m_imageByteBuffer = nullptr;
  this->m_imageUser = nullptr;
  this->m_imagewidth = 0;
  this->m_imageheight = 0;
  this->m_framenumber = 0;
  this->m_depthBuffer = nullptr;
  this->m_numberOfChannels = 0;
  this->m_rd = nullptr;
  this->m_viewName = nullptr;
}
ImageOperation::ImageOperation() : BaseImageOperation()
{
  this->addOutputSocket(DataType::Color);
}
ImageAlphaOperation::ImageAlphaOperation() : BaseImageOperation()
{
  this->addOutputSocket(DataType::Value);
}
ImageDepthOperation::ImageDepthOperation() : BaseImageOperation()
{
  this->addOutputSocket(DataType::Value);
}

ImBuf *BaseImageOperation::getImBuf()
{
  ImBuf *ibuf;
  ImageUser iuser = *this->m_imageUser;

  if (this->m_image == nullptr) {
    return nullptr;
  }

  /* local changes to the original ImageUser */
  if (BKE_image_is_multilayer(this->m_image) == false) {
    iuser.multi_index = BKE_scene_multiview_view_id_get(this->m_rd, this->m_viewName);
  }

  ibuf = BKE_image_acquire_ibuf(this->m_image, &iuser, nullptr);
  if (ibuf == nullptr || (ibuf->rect == nullptr && ibuf->rect_float == nullptr)) {
    BKE_image_release_ibuf(this->m_image, ibuf, nullptr);
    return nullptr;
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
}

void BaseImageOperation::deinitExecution()
{
  this->m_imageFloatBuffer = nullptr;
  this->m_imageByteBuffer = nullptr;
  BKE_image_release_ibuf(this->m_image, this->m_buffer, nullptr);
}

void BaseImageOperation::determineResolution(unsigned int resolution[2],
                                             unsigned int /*preferredResolution*/[2])
{
  ImBuf *stackbuf = getImBuf();

  resolution[0] = 0;
  resolution[1] = 0;

  if (stackbuf) {
    resolution[0] = stackbuf->x;
    resolution[1] = stackbuf->y;
  }

  BKE_image_release_ibuf(this->m_image, stackbuf, nullptr);
}

static void sampleImageAtLocation(
    ImBuf *ibuf, float x, float y, PixelSampler sampler, bool make_linear_rgb, float color[4])
{
  if (ibuf->rect_float) {
    switch (sampler) {
      case PixelSampler::Nearest:
        nearest_interpolation_color(ibuf, nullptr, color, x, y);
        break;
      case PixelSampler::Bilinear:
        bilinear_interpolation_color(ibuf, nullptr, color, x, y);
        break;
      case PixelSampler::Bicubic:
        bicubic_interpolation_color(ibuf, nullptr, color, x, y);
        break;
    }
  }
  else {
    unsigned char byte_color[4];
    switch (sampler) {
      case PixelSampler::Nearest:
        nearest_interpolation_color(ibuf, byte_color, nullptr, x, y);
        break;
      case PixelSampler::Bilinear:
        bilinear_interpolation_color(ibuf, byte_color, nullptr, x, y);
        break;
      case PixelSampler::Bicubic:
        bicubic_interpolation_color(ibuf, byte_color, nullptr, x, y);
        break;
    }
    rgba_uchar_to_float(color, byte_color);
    if (make_linear_rgb) {
      IMB_colormanagement_colorspace_to_scene_linear_v4(color, false, ibuf->rect_colorspace);
    }
  }
}

static void copyImBufRect(const rcti &render_rect,
                          ImBuf &im_buf,
                          CPUBuffer<float> &output,
                          int ch_offset,
                          int im_buf_elem_chs,
                          int im_buf_elem_chs_stride)
{
  int width = im_buf.x;
  if (im_buf.rect_float) {
    for (int y = render_rect.ymin; y < render_rect.ymax; y++) {
      int x = render_rect.xmin;
      float *output_elem = output.getElem(x, y);
      float *input_elem = im_buf.rect_float + y * width * im_buf_elem_chs_stride +
                          x * im_buf_elem_chs_stride;
      for (; x < render_rect.xmax; x++) {
        for (int ch = ch_offset; ch < im_buf_elem_chs; ch++) {
          output_elem[ch] = input_elem[ch];
        }

        output_elem += output.elem_jump;
        input_elem += im_buf_elem_chs_stride;
      }
    }
  }
  else {
    for (int y = render_rect.ymin; y < render_rect.ymax; y++) {
      int x = render_rect.xmin;
      float *output_elem = output.getElem(x, y);
      unsigned char *input_elem = ((unsigned char *)im_buf.rect) +
                                  y * width * im_buf_elem_chs_stride + x * im_buf_elem_chs_stride;
      for (; x < render_rect.xmax; x++) {
        rgba_uchar_to_float(output_elem, input_elem);
        output_elem += output.elem_jump;
        input_elem += im_buf_elem_chs_stride;
      }
    }
  }
}

void ImageOperation::execPixelsMultiCPU(const rcti &render_rect,
                                        CPUBuffer<float> &output,
                                        blender::Span<const CPUBuffer<float> *> inputs,
                                        ExecutionSystem *exec_system,
                                        int current_pass)
{
  if (m_buffer) {
    constexpr int n_chs = COM_data_type_num_channels(DataType::Color);
    int width = BLI_rcti_size_x(&render_rect);
    copyImBufRect(render_rect, *m_buffer, output, 0, n_chs, n_chs);

    /* Convert colorspace to scene linear per row */
    for (int y = render_rect.ymin; y < render_rect.ymax; y++) {
      int x = render_rect.xmin;
      float *output_elem = output.getElem(x, y);
      IMB_colormanagement_colorspace_to_scene_linear(
          output_elem, width, 1, n_chs, m_buffer->rect_colorspace, false);
    }
  }
}

void ImageAlphaOperation::execPixelsMultiCPU(const rcti &render_rect,
                                             CPUBuffer<float> &output,
                                             blender::Span<const CPUBuffer<float> *> inputs,
                                             ExecutionSystem *exec_system,
                                             int current_pass)
{
  if (m_buffer) {
    copyImBufRect(render_rect, *m_buffer, output, 3, 1, 1);
  }
}

void ImageDepthOperation::execPixelsMultiCPU(const rcti &render_rect,
                                             CPUBuffer<float> &output,
                                             blender::Span<const CPUBuffer<float> *> inputs,
                                             ExecutionSystem *exec_system,
                                             int current_pass)
{
  if (m_depthBuffer) {
    int width = BLI_rcti_size_x(&render_rect);
    int height = BLI_rcti_size_y(&render_rect);
    for (int y = render_rect.ymin; y < render_rect.ymax; y++) {
      int x = render_rect.xmin;
      float *output_elem = output.getElem(x, y);
      float *input_elem = m_depthBuffer + y * width + x * height;
      for (; x < render_rect.xmax; x++) {
        output_elem += output.elem_jump;
        input_elem++;
      }
    }
  }
}

}  // namespace blender::compositor
