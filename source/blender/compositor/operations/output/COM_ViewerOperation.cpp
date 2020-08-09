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

#include "COM_ViewerOperation.h"
#include "BKE_global.h"
#include "BKE_image.h"
#include "BKE_scene.h"
#include "BLI_listbase.h"
#include "BLI_math_color.h"
#include "BLI_math_vector.h"
#include "BLI_threads.h"
#include "BLI_utildefines.h"
#include "COM_Buffer.h"
#include "COM_BufferUtil.h"
#include "DNA_node_types.h"
#include "MEM_guardedalloc.h"
#include "PIL_time.h"
#include "WM_api.h"
#include "WM_types.h"

#include "COM_GlobalManager.h"
#include "COM_PixelsUtil.h"
#include "IMB_colormanagement.h"
#include "IMB_imbuf.h"
#include "IMB_imbuf_types.h"

ViewerOperation::ViewerOperation() : NodeOperation()
{
  this->setImage(NULL);
  this->setImageUser(NULL);
  this->m_outputBuffer = NULL;
  this->m_depthBuffer = NULL;
  this->m_active = false;
  this->m_doDepthBuffer = false;
  this->m_viewSettings = NULL;
  this->m_displaySettings = NULL;
  this->m_useAlphaInput = false;

  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);

  this->m_rd = NULL;
  this->m_viewName = NULL;
  m_needs_write = false;
}

void ViewerOperation::initExecution()
{
  m_needs_write = GlobalMan->ViewCacheMan->viewerNeedsUpdate(this);
  m_doDepthBuffer = getInputSocket(2)->hasUserLink();
  if (isActiveViewerOutput()) {
    initImage();
  }
}

void ViewerOperation::deinitExecution()
{
  this->m_outputBuffer = NULL;
  this->m_depthBuffer = NULL;
  if (!isBreaked() && m_needs_write) {
    GlobalMan->ViewCacheMan->reportViewerWrite(this);
  }
}

bool ViewerOperation::isOutputOperation(bool /*rendering*/) const
{
  if (G.background) {
    return false;
  }
  return isActiveViewerOutput();
}

void ViewerOperation::execPixels(ExecutionManager &man)
{
  auto src_color = getInputOperation(0)->getPixels(this, man);
  std::shared_ptr<PixelsRect> src_depth;
  if (m_doDepthBuffer) {
    src_depth = getInputOperation(2)->getPixels(this, man);
  }
  std::shared_ptr<PixelsRect> src_alpha;
  if (m_useAlphaInput) {
    src_alpha = getInputOperation(1)->getPixels(this, man);
  }
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    auto img_buf = BufferUtil::createUnmanagedTmpBuffer(
        COM_NUM_CHANNELS_COLOR, m_outputBuffer, getWidth(), getHeight(), false);
    PixelsRect img_dst(img_buf.get(), dst);
    PixelsRect color_rect = src_color->toRect(dst);
    PixelsUtil::copyEqualRects(img_dst, color_rect);
    if (m_doDepthBuffer) {
      auto dep_buf = BufferUtil::createUnmanagedTmpBuffer(
          COM_NUM_CHANNELS_VALUE, m_depthBuffer, getWidth(), getHeight(), false);
      PixelsRect depth_rect = src_depth->toRect(dst);
      PixelsRect dep_dst(dep_buf.get(), dst);
      PixelsUtil::copyEqualRects(dep_dst, depth_rect);
    }

    if (m_useAlphaInput) {
      PixelsRect alpha_rect = src_alpha->toRect(dst);
      PixelsUtil::copyEqualRectsChannel(img_dst, 3, alpha_rect, 0);
    }

    updateImage(&dst);
  };
  cpuWriteSeek(man, cpuWrite);
}

void ViewerOperation::initImage()
{
  Image *ima = this->m_image;
  ImageUser iuser = *this->m_imageUser;
  void *lock;
  ImBuf *ibuf;

  /* make sure the image has the correct number of views */
  if (ima && BKE_scene_multiview_is_render_view_first(this->m_rd, this->m_viewName)) {
    BKE_image_ensure_viewer_views(this->m_rd, ima, this->m_imageUser);
  }

  BLI_thread_lock(LOCK_DRAW_IMAGE);

  /* local changes to the original ImageUser */
  iuser.multi_index = BKE_scene_multiview_view_id_get(this->m_rd, this->m_viewName);
  ibuf = BKE_image_acquire_ibuf(ima, &iuser, &lock);

  if (!ibuf) {
    BLI_thread_unlock(LOCK_DRAW_IMAGE);
    return;
  }
  if (ibuf->x != (int)getWidth() || ibuf->y != (int)getHeight()) {

    imb_freerectImBuf(ibuf);
    imb_freerectfloatImBuf(ibuf);
    IMB_freezbuffloatImBuf(ibuf);
    ibuf->x = getWidth();
    ibuf->y = getHeight();
    /* zero size can happen if no image buffers exist to define a sensible resolution */
    if (ibuf->x > 0 && ibuf->y > 0) {
      imb_addrectfloatImBuf(ibuf);
    }
    ImageTile *tile = BKE_image_get_tile(ima, 0);
    tile->ok = IMA_OK_LOADED;

    ibuf->userflags |= IB_DISPLAY_BUFFER_INVALID;
  }

  if (m_doDepthBuffer) {
    addzbuffloatImBuf(ibuf);
  }

  /* now we combine the input with ibuf */
  this->m_outputBuffer = ibuf->rect_float;

  /* needed for display buffer update */
  this->m_ibuf = ibuf;

  if (m_doDepthBuffer) {
    this->m_depthBuffer = ibuf->zbuf_float;
  }

  BKE_image_release_ibuf(this->m_image, this->m_ibuf, lock);

  BLI_thread_unlock(LOCK_DRAW_IMAGE);
}

void ViewerOperation::updateImage(const rcti *rect)
{
  IMB_partial_display_buffer_update(this->m_ibuf,
                                    this->m_outputBuffer,
                                    NULL,
                                    getWidth(),
                                    0,
                                    0,
                                    this->m_viewSettings,
                                    this->m_displaySettings,
                                    rect->xmin,
                                    rect->ymin,
                                    rect->xmax,
                                    rect->ymax);

  this->updateDraw();
}
//
// void ViewerOperation::determineResolution(int resolution[2],
//                                          int preferredResolution[2],
//                                          DetermineResolutionMode mode,
//                                          bool setResolution)
//{
//  NodeOperation::determineResolution(
//      resolution, preferredResolution, DetermineResolutionMode::FromInput, false);
//  int width = resolution[0];
//  int height = resolution[1];
//  float divider = 0.0f;
//  if (width == 0 || height == 0) {
//    resolution[0] = 0;
//    resolution[1] = 0;
//  }
//  else {
//    if (width > height) {
//      divider = COM_PREVIEW_SIZE / width;
//    }
//    else {
//      divider = COM_PREVIEW_SIZE / height;
//    }
//    width = width * divider;
//    height = height * divider;
//
//    resolution[0] = width;
//    resolution[1] = height;
//
//    int temp_res[2] = {0, 0};
//    int local_preferred[2] = {width, height};
//    NodeOperation::determineResolution(
//        temp_res, local_preferred, DetermineResolutionMode::FromOutput, true);
//  }
//}
