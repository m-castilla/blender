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

#include "COM_PreviewOperation.h"
#include "BKE_global.h"
#include "BKE_image.h"
#include "BLI_listbase.h"
#include "BLI_math.h"
#include "BLI_math_color.h"
#include "BLI_utildefines.h"
#include "COM_CompositorContext.h"
#include "COM_defines.h"
#include "MEM_guardedalloc.h"
#include "PIL_time.h"
#include "WM_api.h"
#include "WM_types.h"

#include "BKE_node.h"
#include "COM_GlobalManager.h"
#include "COM_PixelsUtil.h"
#include "IMB_colormanagement.h"
#include "IMB_imbuf.h"
#include "IMB_imbuf_types.h"

PreviewOperation::PreviewOperation(const ColorManagedViewSettings *viewSettings,
                                   const ColorManagedDisplaySettings *displaySettings)
    : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR, InputResizeMode::NO_RESIZE);
  this->m_preview = NULL;
  this->m_outputBuffer = NULL;
  this->m_viewSettings = viewSettings;
  this->m_displaySettings = displaySettings;
  this->m_needs_write = false;
  this->m_multiplier = 0.0f;
}

void PreviewOperation::verifyPreview(bNodeInstanceHash *previews, bNodeInstanceKey key)
{
  /* Size (0, 0) ensures the preview rect is not allocated in advance,
   * this is set later in initExecution once the resolution is determined.
   */
  m_preview = BKE_node_preview_verify(previews, key, 0, 0, true);
}

void PreviewOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(getPreviewKey());
}

bool PreviewOperation::isOutputOperation(bool /*rendering*/) const
{
  return !G.background;
}

unsigned char *PreviewOperation::createBuffer()
{
  return (unsigned char *)MEM_mallocN(getBufferBytes(), "PreviewOperation");
}

void PreviewOperation::initExecution()
{
  m_preview->xsize = getWidth();
  m_preview->ysize = getHeight();
  bool has_cache = GlobalMan->ViewCacheMan->getPreviewCache(this) != nullptr;
  if (has_cache) {
    m_needs_write = false;
  }
  else {
    this->m_outputBuffer = createBuffer();
    m_needs_write = true;
  }
}

void PreviewOperation::deinitExecution()
{
  unsigned char *preview_buffer = nullptr;
  if (m_needs_write) {
    if (isBreaked()) {
      if (m_outputBuffer) {
        MEM_freeN(m_outputBuffer);
      }
    }
    else {
      preview_buffer = createBuffer();
      memcpy(preview_buffer, m_outputBuffer, getBufferBytes());
      GlobalMan->ViewCacheMan->reportPreviewWrite(this, m_outputBuffer);
    }
  }

  // set preview
  if (preview_buffer == nullptr) {
    auto cache = GlobalMan->ViewCacheMan->getPreviewCache(this);
    if (cache) {
      preview_buffer = createBuffer();
      memcpy(preview_buffer, cache, getBufferBytes());
    }
  }
  if (preview_buffer && m_preview->rect) {
    MEM_freeN(this->m_preview->rect);
  }
  this->m_preview->rect = preview_buffer;

  this->m_outputBuffer = nullptr;
}

void PreviewOperation::execPixels(ExecutionManager &man)
{
  int width = getWidth();
  int height = getHeight();
  auto src_pixels = getInputOperation(0)->getPixels(this, man);

  auto cpuWrite = [=](PixelsRect &dst, const WriteRectContext &ctx) {
    float color[4];
    struct ColormanageProcessor *cm_processor;
    cm_processor = IMB_colormanagement_display_processor_new(this->m_viewSettings,
                                                             this->m_displaySettings);

    auto img = src_pixels->pixelsImg();
    PixelsSampler sampler = PixelsSampler{PixelInterpolation::NEAREST, PixelExtend::CLIP};
    size_t dst_offset;
    for (int y = 0; y < height; y++) {
      dst_offset = y * (size_t)width * COM_NUM_CHANNELS_COLOR;
      for (int x = 0; x < width; x++) {
        img.sample(color, sampler, x * m_multiplier, y * m_multiplier);
        IMB_colormanagement_processor_apply_v4(cm_processor, color);
        unit_float_to_uchar_clamp_v4(this->m_outputBuffer + dst_offset, color);
        dst_offset += COM_NUM_CHANNELS_COLOR;
      }
    }

    IMB_colormanagement_processor_free(cm_processor);
  };
  return cpuWriteSeek(man, cpuWrite);
}

void PreviewOperation::determineResolution(int resolution[2],
                                           int preferredResolution[2],
                                           bool setResolution)
{
  NodeOperation::determineResolution(resolution, preferredResolution, setResolution);
  int width = resolution[0];
  int height = resolution[1];
  float divider = 0.0f;
  if (width == 0 || height == 0) {
    resolution[0] = 0;
    resolution[1] = 0;
  }
  else {
    if (width > height) {
      divider = COM_PREVIEW_SIZE / width;
    }
    else {
      divider = COM_PREVIEW_SIZE / height;
    }
    width = width * divider;
    height = height * divider;
    m_multiplier = 1.0f / divider;

    resolution[0] = width;
    resolution[1] = height;
  }
}
