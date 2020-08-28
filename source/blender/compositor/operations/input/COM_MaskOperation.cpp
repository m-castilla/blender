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
 * Copyright 2012, Blender Foundation.
 */
#include <cstring>

#include "COM_MaskOperation.h"
#include "BKE_mask.h"
#include "BLI_listbase.h"
#include "MEM_guardedalloc.h"

#include "COM_kernel_cpu.h"

MaskOperation::MaskOperation() : NodeOperation()
{
  this->addOutputSocket(SocketType::VALUE);
  this->m_mask = NULL;
  this->m_maskWidth = 0;
  this->m_maskHeight = 0;
  this->m_maskWidthInv = 0.0f;
  this->m_maskHeightInv = 0.0f;
  this->m_frame_shutter = 0.0f;
  this->m_frame_number = 0;
  this->m_rasterMaskHandleTot = 1;
  memset(this->m_rasterMaskHandles, 0, sizeof(this->m_rasterMaskHandles));
}

void MaskOperation::initExecution()
{
  if (this->m_mask && this->m_rasterMaskHandles[0] == NULL) {
    if (this->m_rasterMaskHandleTot == 1) {
      this->m_rasterMaskHandles[0] = BKE_maskrasterize_handle_new();

      BKE_maskrasterize_handle_init(this->m_rasterMaskHandles[0],
                                    this->m_mask,
                                    this->m_maskWidth,
                                    this->m_maskHeight,
                                    true,
                                    true,
                                    this->m_do_feather);
    }
    else {
      /* make a throw away copy of the mask */
      const float frame = (float)this->m_frame_number - this->m_frame_shutter;
      const float frame_step = (this->m_frame_shutter * 2.0f) / this->m_rasterMaskHandleTot;
      float frame_iter = frame;

      Mask *mask_temp;

      mask_temp = BKE_mask_copy_nolib(this->m_mask);

      /* trick so we can get unkeyed edits to display */
      {
        MaskLayer *masklay;
        MaskLayerShape *masklay_shape;

        for (masklay = (MaskLayer *)mask_temp->masklayers.first; masklay;
             masklay = masklay->next) {
          masklay_shape = BKE_mask_layer_shape_verify_frame(masklay, this->m_frame_number);
          BKE_mask_layer_shape_from_mask(masklay, masklay_shape);
        }
      }

      for (unsigned int i = 0; i < this->m_rasterMaskHandleTot; i++) {
        this->m_rasterMaskHandles[i] = BKE_maskrasterize_handle_new();

        /* re-eval frame info */
        BKE_mask_evaluate(mask_temp, frame_iter, true);

        BKE_maskrasterize_handle_init(this->m_rasterMaskHandles[i],
                                      mask_temp,
                                      this->m_maskWidth,
                                      this->m_maskHeight,
                                      true,
                                      true,
                                      this->m_do_feather);

        frame_iter += frame_step;
      }

      BKE_mask_free(mask_temp);
      MEM_freeN(mask_temp);
    }
  }
  NodeOperation::initExecution();
}

void MaskOperation::deinitExecution()
{
  for (unsigned int i = 0; i < this->m_rasterMaskHandleTot; i++) {
    if (this->m_rasterMaskHandles[i]) {
      BKE_maskrasterize_handle_free(this->m_rasterMaskHandles[i]);
      this->m_rasterMaskHandles[i] = NULL;
    }
  }
}

ResolutionType MaskOperation::determineResolution(int resolution[2],
                                                  int preferredResolution[2],

                                                  bool setResolution)
{
  if (this->m_maskWidth == 0 || this->m_maskHeight == 0) {
    NodeOperation::determineResolution(resolution, preferredResolution, setResolution);
    return ResolutionType::Determined;
  }
  else {
    int nr[2];

    nr[0] = this->m_maskWidth;
    nr[1] = this->m_maskHeight;

    if (setResolution) {
      NodeOperation::determineResolution(resolution, nr, setResolution);
    }

    resolution[0] = this->m_maskWidth;
    resolution[1] = this->m_maskHeight;
    return ResolutionType::Fixed;
  }
}

void MaskOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_maskWidth);
  hashParam(m_maskHeight);
  hashParam(m_frame_number);
  hashParam(m_frame_shutter);
  hashParam(m_rasterMaskHandleTot);
  hashParam(m_do_feather);
  if (m_mask) {
    hashParam(m_mask->id.session_uuid);
    hashParam(m_mask->masklay_act);
  }
}

void MaskOperation::execPixels(ExecutionManager &man)
{
  auto cpu_write = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    WRITE_DECL(dst);

    CPU_LOOP_START(dst);

    const float xy[2] = {
        (dst_coords.x * m_maskWidthInv) + m_mask_px_ofs[0],
        (dst_coords.y * m_maskHeightInv) + m_mask_px_ofs[1],
    };

    float result = 0.0f;
    if (this->m_rasterMaskHandleTot == 1) {
      if (this->m_rasterMaskHandles[0]) {
        result = BKE_maskrasterize_handle_sample(this->m_rasterMaskHandles[0], xy);
      }
    }
    else {
      for (unsigned int i = 0; i < this->m_rasterMaskHandleTot; i++) {
        if (this->m_rasterMaskHandles[i]) {
          result += BKE_maskrasterize_handle_sample(this->m_rasterMaskHandles[i], xy);
        }
      }

      /* until we get better falloff */
      result /= this->m_rasterMaskHandleTot;
    }

    dst_img.buffer[dst_offset] = result;

    CPU_LOOP_END;
  };

  cpuWriteSeek(man, cpu_write);
}
