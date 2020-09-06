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
  this->m_inputBuffer = NULL;
  this->m_elementsize = elementsize;
  this->m_rd = NULL;

  this->addOutputSocket(PixelsUtil::dataToSocketType(type));
}

void RenderLayersProg::initExecution()
{
  Scene *scene = this->getScene();
  Render *re = (scene) ? RE_GetSceneRender(scene) : NULL;
  RenderResult *rr = NULL;

  if (re) {
    rr = RE_AcquireResultRead(re);
  }

  if (rr) {
    ViewLayer *view_layer = (ViewLayer *)BLI_findlink(&scene->view_layers, getLayerId());
    if (view_layer) {

      RenderLayer *rl = RE_GetRenderLayer(rr, view_layer->name);
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
  hashParam(m_layerId);
  hashParam(std::string(m_passName));
  hashParam(std::string(m_viewName));
}

void RenderLayersProg::deinitExecution()
{
  this->m_inputBuffer = NULL;
}

TmpBuffer *RenderLayersProg::getCustomBuffer()
{
  return BufferUtil::createNonStdTmpBuffer(
             m_inputBuffer, true, getWidth(), getHeight(), m_elementsize)
      .release();
}

ResolutionType RenderLayersProg::determineResolution(int resolution[2],
                                                     int /*preferredResolution*/[2],
                                                     bool /*setResolution*/)
{
  Scene *sce = this->getScene();
  Render *re = (sce) ? RE_GetSceneRender(sce) : NULL;
  RenderResult *rr = NULL;

  resolution[0] = 0;
  resolution[1] = 0;

  if (re) {
    rr = RE_AcquireResultRead(re);
  }

  if (rr) {
    ViewLayer *view_layer = (ViewLayer *)BLI_findlink(&sce->view_layers, getLayerId());
    if (view_layer) {
      RenderLayer *rl = RE_GetRenderLayer(rr, view_layer->name);
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
