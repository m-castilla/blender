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

#include "COM_RenderLayersProg.h"
#include "BKE_scene.h"
#include "BLI_listbase.h"
#include "COM_Buffer.h"
#include "COM_BufferUtil.h"
#include "COM_CompositorContext.h"
#include "COM_GlobalManager.h"
#include "COM_PixelsUtil.h"
#include "DNA_scene_types.h"
#include "RE_pipeline.h"
#include "RE_render_ext.h"
#include "RE_shader_ext.h"

/* ******** Render Layers Base Prog ******** */

RenderLayersProg::RenderLayersProg(const char *passName, DataType type, int elementsize)
    : NodeOperation(), m_passName(passName)
{
  this->setScene(NULL);
  this->setViewLayer(NULL);
  m_layer_id = 0;
  this->m_inputBuffer = NULL;
  this->m_elementsize = elementsize;
  this->m_rd = NULL;

  this->addOutputSocket(PixelsUtil::dataToSocketType(type));
}

void RenderLayersProg::initExecution()
{
  auto ctx = GlobalMan->getContext();
  Render *re = ctx->renderer()->getRender(ctx, m_scene, m_view_layer);
  RenderResult *rr = NULL;

  if (re) {
    rr = RE_AcquireResultRead(re);
  }

  if (rr) {
    if (m_view_layer) {
      RenderLayer *rl = RE_GetRenderLayer(rr, m_view_layer->name);
      if (rl) {
        m_inputBuffer = RE_RenderLayerGetPass(rl, this->m_passName.c_str(), this->m_viewName);
      }
    }
  }
  if (re) {
    RE_ReleaseResult(re);
    re = NULL;
  }
  NodeOperation::initExecution();
}

void RenderLayersProg::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_inputBuffer);
  hashParam(m_layer_id);
  hashParam(std::string(m_passName));
  hashParam(std::string(m_viewName));
}

void RenderLayersProg::deinitExecution()
{
  this->m_inputBuffer = NULL;
  NodeOperation::deinitExecution();
}

TmpBuffer *RenderLayersProg::getCustomBuffer()
{
  int w = getWidth();
  int h = getHeight();
  auto recycler = GlobalMan->BufferMan->recycler();
  if (m_elementsize == COM_NUM_CHANNELS_STD) {
    return recycler->createStdTmpBuffer(false, m_inputBuffer, w, h, m_elementsize);
  }
  else {
    auto tmp = recycler->createTmpBuffer(true);
    recycler->takeStdRecycle(BufferRecycleType::HOST_CLEAR, tmp, w, h, m_elementsize);
    PixelsRect tmp_rect = PixelsRect(tmp, 0, w, 0, h);
    PixelsUtil::copyBufferRectNChannels(tmp_rect, m_inputBuffer, m_elementsize, m_elementsize);
    return tmp;
  }
}

ResolutionType RenderLayersProg::determineResolution(int resolution[2],
                                                     int /*preferredResolution*/[2],
                                                     bool /*setResolution*/)
{
  auto ctx = GlobalMan->getContext();
  Render *re = ctx->renderer()->getRender(ctx, m_scene, m_view_layer);
  RenderResult *rr = NULL;

  resolution[0] = 0;
  resolution[1] = 0;

  if (re) {
    rr = RE_AcquireResultRead(re);
  }

  if (rr) {

    if (m_view_layer) {
      RenderLayer *rl = RE_GetRenderLayer(rr, m_view_layer->name);
      if (rl) {
        resolution[0] = rl->rectx;
        resolution[1] = rl->recty;
      }
    }
  }

  if (re) {
    RE_ReleaseResult(re);
  }

  return ResolutionType::Fixed;
}

/* ******** Render Layers Alpha Operation ******** */
void RenderLayersAlphaProg::execPixels(ExecutionManager &man)
{
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    if (m_inputBuffer == NULL) {
      PixelsUtil::setRectElem(dst, 1.0f);
    }
    else {
      PixelsUtil::copyBufferRectChannel(dst, 0, m_inputBuffer, 3, COM_NUM_CHANNELS_COLOR);
    }
  };
  cpuWriteSeek(man, cpuWrite);
}
