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

#include "COM_CompositorOperation.h"
#include "COM_GlobalManager.h"

#include "BKE_global.h"
#include "BKE_image.h"
#include "BLI_listbase.h"
#include "MEM_guardedalloc.h"

#include "BLI_threads.h"

#include "RE_pipeline.h"
#include "RE_render_ext.h"
#include "RE_shader_ext.h"

#include "render_types.h"

#include "COM_Buffer.h"
#include "COM_BufferUtil.h"
#include "COM_PixelsUtil.h"
#include "PIL_time.h"

CompositorOperation::CompositorOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);

  this->setRenderData(NULL);
  this->m_outputBuffer = NULL;
  this->m_depthBuffer = NULL;

  this->m_useAlphaInput = false;
  this->m_active = false;

  this->m_scene = NULL;
  this->m_sceneName[0] = '\0';
  this->m_viewName = NULL;
}

void CompositorOperation::initExecution()
{
  if (!this->m_active) {
    return;
  }

  // When initializing the tree during initial load the width and height can be zero.
  if (this->getWidth() * this->getHeight() != 0) {
    this->m_outputBuffer = (float *)MEM_mallocN(
        (size_t)this->getWidth() * this->getHeight() * 4 * sizeof(float), "CompositorOperation");
    this->m_depthBuffer = (float *)MEM_mallocN(
        (size_t)this->getWidth() * this->getHeight() * sizeof(float), "CompositorOperation");
  }
  NodeOperation::initExecution();
}

void CompositorOperation::deinitExecution()
{
  if (!this->m_active) {
    return;
  }

  if (!GlobalMan->getContext()->isBreaked()) {
    Render *re = RE_GetSceneRender(this->m_scene);
    RenderResult *rr = RE_AcquireResultWrite(re);

    if (rr) {
      RenderView *rv = RE_RenderViewGetByName(rr, this->m_viewName);

      if (rv->rectf != NULL) {
        MEM_freeN(rv->rectf);
      }
      rv->rectf = this->m_outputBuffer;
      if (rv->rectz != NULL) {
        MEM_freeN(rv->rectz);
      }
      rv->rectz = this->m_depthBuffer;
      rr->have_combined = true;
    }
    else {
      if (this->m_outputBuffer) {
        MEM_freeN(this->m_outputBuffer);
      }
      if (this->m_depthBuffer) {
        MEM_freeN(this->m_depthBuffer);
      }
    }

    if (re) {
      RE_ReleaseResult(re);
      re = NULL;
    }

    BLI_thread_lock(LOCK_DRAW_IMAGE);
    BKE_image_signal(G.main,
                     BKE_image_ensure_viewer(G.main, IMA_TYPE_R_RESULT, "Render Result"),
                     NULL,
                     IMA_SIGNAL_FREE);
    BLI_thread_unlock(LOCK_DRAW_IMAGE);
  }
  else {
    if (this->m_outputBuffer) {
      MEM_freeN(this->m_outputBuffer);
    }
    if (this->m_depthBuffer) {
      MEM_freeN(this->m_depthBuffer);
    }
  }

  this->m_outputBuffer = NULL;
  this->m_depthBuffer = NULL;
  NodeOperation::deinitExecution();
}

void CompositorOperation::execPixels(ExecutionManager &man)
{
  auto image = getInputOperation(0)->getPixels(this, man);
  auto depth = getInputOperation(2)->getPixels(this, man);
  auto alpha = std::shared_ptr<PixelsRect>();
  if (m_useAlphaInput) {
    alpha = getInputOperation(1)->getPixels(this, man);
  }
  auto cpuWrite = [&, this](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    float *out = this->m_outputBuffer;
    float *z = this->m_depthBuffer;
    PixelsRect image_rect = image->toRect(dst);
    PixelsUtil::copyBufferRect(
        out, dst, COM_NUM_CHANNELS_COLOR, COM_NUM_CHANNELS_COLOR, image_rect);

    PixelsRect depth_rect = depth->toRect(dst);
    PixelsUtil::copyBufferRect(z, dst, COM_NUM_CHANNELS_VALUE, COM_NUM_CHANNELS_VALUE, depth_rect);

    if (m_useAlphaInput) {
      PixelsRect alpha_rect = alpha->toRect(dst);
      PixelsUtil::copyBufferRectChannel(out, 3, dst, COM_NUM_CHANNELS_COLOR, alpha_rect, 0);
    }
  };
  cpuWriteSeek(man, cpuWrite);
}

ResolutionType CompositorOperation::determineResolution(int resolution[2],
                                                        int preferredResolution[2],
                                                        bool setResolution)
{
  int width = this->m_rd->xsch * this->m_rd->size / 100;
  int height = this->m_rd->ysch * this->m_rd->size / 100;

  // check actual render resolution with cropping it may differ with cropped border.rendering
  // FIX for: [31777] Border Crop gives black (easy)
  Render *re = RE_GetSceneRender(this->m_scene);
  if (re) {
    RenderResult *rr = RE_AcquireResultRead(re);
    if (rr) {
      width = rr->rectx;
      height = rr->recty;
    }
    RE_ReleaseResult(re);
  }

  preferredResolution[0] = width;
  preferredResolution[1] = height;

  NodeOperation::determineResolution(resolution, preferredResolution, setResolution);

  resolution[0] = width;
  resolution[1] = height;

  return ResolutionType::Fixed;
}
