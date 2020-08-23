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

#include "IMB_colormanagement.h"

#include "COM_ColorCorrectionOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

ColorCorrectionOperation::ColorCorrectionOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::COLOR);
  this->m_redChannelEnabled = true;
  this->m_greenChannelEnabled = true;
  this->m_blueChannelEnabled = true;
  m_data = nullptr;
}
void ColorCorrectionOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_redChannelEnabled);
  hashParam(m_greenChannelEnabled);
  hashParam(m_blueChannelEnabled);

  hashParam(m_data->startmidtones);
  hashParam(m_data->endmidtones);

  hashParam(m_data->highlights.contrast);
  hashParam(m_data->highlights.gamma);
  hashParam(m_data->highlights.gain);
  hashParam(m_data->highlights.lift);
  hashParam(m_data->highlights.saturation);

  hashParam(m_data->master.contrast);
  hashParam(m_data->master.gamma);
  hashParam(m_data->master.gain);
  hashParam(m_data->master.lift);
  hashParam(m_data->master.saturation);

  hashParam(m_data->midtones.contrast);
  hashParam(m_data->midtones.gamma);
  hashParam(m_data->midtones.gain);
  hashParam(m_data->midtones.lift);
  hashParam(m_data->midtones.saturation);

  hashParam(m_data->shadows.contrast);
  hashParam(m_data->shadows.gamma);
  hashParam(m_data->shadows.gain);
  hashParam(m_data->shadows.lift);
  hashParam(m_data->shadows.saturation);
}

void ColorCorrectionOperation::execPixels(ExecutionManager &man)
{
  auto color = this->getInputOperation(0)->getPixels(this, man);
  auto mask = this->getInputOperation(1)->getPixels(this, man);
  auto cpu_write = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    READ_DECL(color);
    READ_DECL(mask);
    WRITE_DECL(dst);
    CPU_LOOP_START(dst);

    COPY_COORDS(mask, dst_coords);
    COPY_COORDS(color, dst_coords);

    READ_IMG4(color, color_pix);
    READ_IMG1(mask, mask_pix);

    const float level = (color_pix.x + color_pix.y + color_pix.z) / 3.0f;
    float contrast = this->m_data->master.contrast;
    float saturation = this->m_data->master.saturation;
    float gamma = this->m_data->master.gamma;
    float gain = this->m_data->master.gain;
    float lift = this->m_data->master.lift;

    const float value = fminf(1.0f, mask_pix.x);
    const float mvalue = 1.0f - value;

    float levelShadows = 0.0;
    float levelMidtones = 0.0;
    float levelHighlights = 0.0;
#define MARGIN 0.10f
#define MARGIN_DIV (0.5f / MARGIN)
    if (level < this->m_data->startmidtones - MARGIN) {
      levelShadows = 1.0f;
    }
    else if (level < this->m_data->startmidtones + MARGIN) {
      levelMidtones = ((level - this->m_data->startmidtones) * MARGIN_DIV) + 0.5f;
      levelShadows = 1.0f - levelMidtones;
    }
    else if (level < this->m_data->endmidtones - MARGIN) {
      levelMidtones = 1.0f;
    }
    else if (level < this->m_data->endmidtones + MARGIN) {
      levelHighlights = ((level - this->m_data->endmidtones) * MARGIN_DIV) + 0.5f;
      levelMidtones = 1.0f - levelHighlights;
    }
    else {
      levelHighlights = 1.0f;
    }
#undef MARGIN
#undef MARGIN_DIV
    contrast *= (levelShadows * this->m_data->shadows.contrast) +
                (levelMidtones * this->m_data->midtones.contrast) +
                (levelHighlights * this->m_data->highlights.contrast);
    saturation *= (levelShadows * this->m_data->shadows.saturation) +
                  (levelMidtones * this->m_data->midtones.saturation) +
                  (levelHighlights * this->m_data->highlights.saturation);
    gamma *= (levelShadows * this->m_data->shadows.gamma) +
             (levelMidtones * this->m_data->midtones.gamma) +
             (levelHighlights * this->m_data->highlights.gamma);
    gain *= (levelShadows * this->m_data->shadows.gain) +
            (levelMidtones * this->m_data->midtones.gain) +
            (levelHighlights * this->m_data->highlights.gain);
    lift += (levelShadows * this->m_data->shadows.lift) +
            (levelMidtones * this->m_data->midtones.lift) +
            (levelHighlights * this->m_data->highlights.lift);

    float invgamma = 1.0f / gamma;
    float luma = IMB_colormanagement_get_luminance((float *)&color_pix);

    CCL::float4 res = (luma + saturation * (color_pix - luma));
    res = 0.5f + ((res - 0.5f) * contrast);

    /* Check for negative values to avoid nan. */
    CCL::float4 pow_base = res * gain + lift;
    res.x = (pow_base.x < 0.0f) ? res.x : powf(pow_base.x, invgamma);
    res.y = (pow_base.y < 0.0f) ? res.y : powf(pow_base.y, invgamma);
    res.z = (pow_base.z < 0.0f) ? res.z : powf(pow_base.z, invgamma);

    // mix with mask
    res = mvalue * color_pix + value * res;

    if (!this->m_redChannelEnabled) {
      res.x = color_pix.x;
    }
    if (!this->m_greenChannelEnabled) {
      res.y = color_pix.y;
    }
    if (!this->m_blueChannelEnabled) {
      res.z = color_pix.z;
    }
    res.w = color_pix.w;

    WRITE_IMG4(dst, res);

    CPU_LOOP_END;
  };
  cpuWriteSeek(man, cpu_write);
}
