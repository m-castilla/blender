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
#include "COM_BufferUtil.h"
#include "COM_GlobalManager.h"
#include "COM_PixelsUtil.h"
#include "IMB_colormanagement.h"
#include "IMB_imbuf.h"
#include "IMB_imbuf_types.h"

#include "COM_ExecutionManager.h"
#include "COM_kernel_cpu.h"

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

unsigned char *PreviewOperation::createPreviewBuffer()
{
  return (unsigned char *)MEM_mallocN(getBufferBytes(), "PreviewOperation");
}

void PreviewOperation::initExecution()
{
  m_preview->xsize = getWidth();
  m_preview->ysize = getHeight();

  NodeOperation::initExecution();
  bool has_cache = GlobalMan->ViewCacheMan->getPreviewCache(this) != nullptr;
  if (has_cache) {
    m_needs_write = false;
  }
  else {
    this->m_outputBuffer = createPreviewBuffer();
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
      preview_buffer = createPreviewBuffer();
      memcpy(preview_buffer, m_outputBuffer, getBufferBytes());
      GlobalMan->ViewCacheMan->reportPreviewWrite(this, m_outputBuffer);
    }
  }

  // set preview
  if (preview_buffer == nullptr) {
    auto cache = GlobalMan->ViewCacheMan->getPreviewCache(this);
    if (cache) {
      preview_buffer = createPreviewBuffer();
      memcpy(preview_buffer, cache, getBufferBytes());
    }
  }
  if (preview_buffer && m_preview->rect) {
    MEM_freeN(this->m_preview->rect);
  }
  this->m_preview->rect = preview_buffer;

  this->m_outputBuffer = nullptr;
  NodeOperation::deinitExecution();
}

void PreviewOperation::execPixels(ExecutionManager &man)
{
  auto src = getInputOperation(0)->getPixels(this, man);
  float scale_x, scale_y;
  if (man.canExecPixels()) {
    scale_x = src->getWidth() / (float)m_width;
    scale_y = src->getHeight() / (float)m_height;
  }
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    int rect_w = dst.getWidth();
    int rect_h = dst.getHeight();

    READ_DECL(src);
    WRITE_DECL(dst);

    CPU_LOOP_START(dst);

    src_coordsf.x = dst_coords.x * scale_x;
    src_coordsf.y = dst_coords.y * scale_y;
    // cast to pixel coords
    SET_COORDS(src, src_coordsf.x, src_coordsf.y);

    READ_IMG4(src, src_pix);

    WRITE_IMG4(dst, src_pix);

    CPU_LOOP_END;

    // auto dst_img = dst.pixelsImg();
    // PixelsRect src_rect = src_pixels->toRect(dst);
    // PixelsUtil::copyEqualRects(dst, src_rect);

    // IMB_colormanagement_processor_apply function don't support row pitch
    BLI_assert(dst_img.row_jump == 0);

    struct ColormanageProcessor *cm_processor = IMB_colormanagement_display_processor_new(
        this->m_viewSettings, this->m_displaySettings);
    IMB_colormanagement_processor_apply(
        cm_processor, dst_img.start, rect_w, rect_h, dst_img.elem_chs, false);
    IMB_colormanagement_processor_free(cm_processor);

    float *float_pixel = dst_img.start;
    unsigned char *uchar_pixel = this->m_outputBuffer +
                                 (size_t)dst.ymin * rect_w * COM_NUM_CHANNELS_COLOR +
                                 (size_t)dst.xmin * COM_NUM_CHANNELS_COLOR;
    while (float_pixel < dst_img.end) {
      unit_float_to_uchar_clamp_v4(uchar_pixel, float_pixel);
      float_pixel += COM_NUM_CHANNELS_COLOR;
      uchar_pixel += COM_NUM_CHANNELS_COLOR;
    }
  };
  return cpuWriteSeek(man, cpuWrite);
}

ResolutionType PreviewOperation::determineResolution(int resolution[2],
                                                     int preferredResolution[2],
                                                     bool /*setResolution*/)
{
  int preview_size = GlobalMan->getContext()->getPreviewSize();
  bool any_user_input = false;
  for (auto input : m_inputs) {
    if (input->hasUserLink()) {
      any_user_input = true;
      break;
    }
  }
  if (any_user_input) {
    preferredResolution[0] = preview_size;
    preferredResolution[1] = preview_size;
  }
  else {
    preferredResolution[0] = 0;
    preferredResolution[1] = 0;
  }
  NodeOperation::determineResolution(resolution, preferredResolution, false);
  int width = resolution[0];
  int height = resolution[1];
  float divider = 0.0f;

  if (width == 0 || height == 0) {
    resolution[0] = 0;
    resolution[1] = 0;
  }
  else {
    if (width > height) {
      divider = preview_size / (float)width;
    }
    else {
      divider = preview_size / (float)height;
    }
    width = width * divider;
    height = height * divider;

    resolution[0] = width;
    resolution[1] = height;

    int temp_res[2] = {0, 0};
    int local_preferred[2] = {width, height};
    NodeOperation::determineResolution(temp_res, local_preferred, true);
  }

  // It's been partially determined by inputs but still it's behavior is more like a fixed one
  return ResolutionType::Fixed;
}
