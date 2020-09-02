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

#include "COM_VideoSequencerOperation.h"
#include "BKE_image.h"
#include "BKE_scene.h"
#include "BKE_sequencer.h"
#include "BLI_listbase.h"
#include "COM_Buffer.h"
#include "COM_BufferUtil.h"
#include "COM_CompositorContext.h"
#include "COM_ExecutionManager.h"
#include "COM_GlobalManager.h"
#include "COM_PixelsUtil.h"
#include "COM_kernel_cpu.h"
#include "DNA_scene_types.h"
#include "DNA_sequence_types.h"
#include "IMB_imbuf.h"
#include "IMB_imbuf_types.h"
#include "IMB_metadata.h"
#include "RE_pipeline.h"
#include "RE_render_ext.h"
#include "RE_shader_ext.h"
#include "render_types.h"

VideoSequencerOperation::VideoSequencerOperation()
    : NodeOperation(), m_seq_frame(nullptr), m_n_frame(0), m_n_channel(0)
{
  this->addOutputSocket(SocketType::COLOR);
  auto context = GlobalMan->getContext();
  m_scene = context->getScene();
  m_is_rendering = context->isRendering();
}

void VideoSequencerOperation::initExecution()
{
  assureSequencerRender();

  NodeOperation::initExecution();
}

void VideoSequencerOperation::hashParams()
{
  NodeOperation::hashParams();
  assureSequencerRender();
  if (m_seq_frame) {
    hashParam((size_t)m_seq_frame->rect_float);
    hashParam((size_t)m_seq_frame->rect);
  }
  hashParam(m_is_rendering);
  hashParam(m_n_frame);
  hashParam(m_n_channel);
}

void VideoSequencerOperation::deinitExecution()
{
  if (m_seq_frame) {
    IMB_freeImBuf(m_seq_frame);
    m_seq_frame = nullptr;
  }
}

void VideoSequencerOperation::assureSequencerRender()
{
  if (!m_seq_frame) {
    auto context = GlobalMan->getContext();
    auto scene = context->getScene();
    m_n_frame = context->getFramenumber();

    Render *render = nullptr;
    if (m_is_rendering) {
      render = RE_GetSceneRender(scene);
    }

    SeqRenderData seq_data;
    if (render) {
      BKE_sequencer_new_render_data(render->main,
                                    render->pipeline_depsgraph,
                                    render->scene,
                                    getWidth(),
                                    getHeight(),
                                    100,
                                    m_is_rendering,
                                    &seq_data);
      RE_ReleaseResult(render);
    }
    else {
      BKE_sequencer_new_render_data(context->getMain(),
                                    context->getDepsgraph(),
                                    scene,
                                    getWidth(),
                                    getHeight(),
                                    100,
                                    m_is_rendering,
                                    &seq_data);
    }
    seq_data.task_id = m_is_rendering ? SEQ_TASK_MAIN_RENDER : SEQ_TASK_PREFETCH_RENDER;

    ImBuf *seq_out = BKE_sequencer_give_ibuf(&seq_data, m_n_frame, m_n_channel);

    if (seq_out) {
      m_seq_frame = seq_out;
      if (m_seq_frame->rect_float) {
        BKE_sequencer_imbuf_from_sequencer_space(scene, m_seq_frame);
      }
    }
  }
}

void VideoSequencerOperation::execPixels(ExecutionManager &man)
{
  float norm_mult = 1.0 / 255.0;
  if (man.canExecPixels()) {
    assureSequencerRender();
  }

  TmpBuffer *dst_buffer = nullptr;
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    dst_buffer = dst.tmp_buffer;
    float *float_buf = m_seq_frame ? m_seq_frame->rect_float : nullptr;
    unsigned int *byte_buf = m_seq_frame ? m_seq_frame->rect : nullptr;
    int n_channels = m_seq_frame ? m_seq_frame->channels : 4;
    if (n_channels == 0) {
      n_channels = 4;
    }
    size_t n_channels_sz = n_channels;
    size_t m_width_sz = dst.getWidth();
    if (float_buf == nullptr && byte_buf == nullptr) {
      PixelsUtil::setRectElem(dst, (float *)&CCL::TRANSPARENT_PIXEL);
    }
    else if (float_buf) {
      auto buf = BufferUtil::createUnmanagedTmpBuffer(
          float_buf, dst.getWidth(), dst.getHeight(), n_channels, true);
      PixelsRect src_rect = PixelsRect(buf.get(), dst);
      PixelsUtil::copyEqualRectsNChannels(dst, src_rect, n_channels);
    }
    else {
      unsigned char *uchar_buf = (unsigned char *)byte_buf;
      CCL::float4 src_pixel;
      size_t src_offset;

      WRITE_DECL(dst);
      CPU_LOOP_START(dst);

      src_offset = n_channels == dst_img.elem_chs ?
                       dst_offset :
                       m_width_sz * dst_coords.y * n_channels_sz + dst_coords.x * n_channels_sz;

      src_pixel = CCL::make_float4(uchar_buf[src_offset],
                                   uchar_buf[src_offset + 1],
                                   uchar_buf[src_offset + 2],
                                   uchar_buf[src_offset + 3]);

      // normalize
      src_pixel *= norm_mult;

      WRITE_IMG4(dst, src_pixel);

      CPU_LOOP_END;
    }
  };
  cpuWriteSeek(man, cpuWrite);

  if (man.getOperationMode() == OperationMode::Exec) {
    // needs colorspace conversion as "BKE_sequencer_imbuf_from_sequencer_space" which has been
    // called in "assureSequencerRender" only works for float buffers
    if (m_seq_frame && !m_seq_frame->rect_float && man.canExecPixels()) {
      Scene *scene = GlobalMan->getContext()->getScene();
      m_seq_frame->rect_float = dst_buffer->host.buffer;
      BKE_sequencer_imbuf_from_sequencer_space(scene, m_seq_frame);
      m_seq_frame->rect_float = nullptr;
    }

    deinitExecution();
  }
}

ResolutionType VideoSequencerOperation::determineResolution(int resolution[2],
                                                            int /*preferredResolution*/[2],
                                                            bool /*setResolution*/)
{
  auto context = GlobalMan->getContext();
  float scale = context->getInputsScale();
  int w = context->getRenderWidth();
  int h = context->getRenderHeight();
  int min_dim = w < h ? w : h;

  // don't allow a resolution lower than 100 pixels (except if min dim is lower) on any dimension
  // to avoid crashes when requesting renders from the sequencer
  float min_scale_size = min_dim > 100 ? 100 : min_dim;
  float min_scale = min_scale_size / (float)min_dim;
  if (min_scale < 0.05f) {
    min_scale = 0.05f;  // don't allow less than 5%, could cause rounding issues when requesting
                        // render from sequencer
  }
  scale = scale > min_scale ? scale : min_scale;
  resolution[0] = w * scale;
  resolution[1] = h * scale;

  return ResolutionType::Determined;
}
