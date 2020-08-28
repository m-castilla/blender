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

#include <cstring>

#include "COM_GlareStreaksOperation.h"
#include "COM_ExecutionManager.h"
#include "COM_GlobalManager.h"
#include "COM_PixelsUtil.h"

#include "COM_kernel_cpu.h"

void GlareStreaksOperation::generateGlare(PixelsRect &dst,
                                          PixelsRect &src,
                                          NodeGlare *settings,
                                          ExecutionManager &man)
{
  int n;
  unsigned int nump = 0;
  CCL::float4 c1, c2, c3, c4;
  float a, ang = DEG2RADF(360.0f) / (float)settings->streaks;
  CCL::float4 zero_f4 = CCL::make_float4_1(0.0f);
  CCL::float4 half_f4 = CCL::make_float4_1(0.5f);
  CCL::float4 result_pix;
  PixelsSampler sampler = {PixelInterpolation::BILINEAR, PixelExtend::CLIP};
  BufferRecycler *recycler = GlobalMan->BufferMan->recycler();

  int src_height = src.getHeight();
  int src_width = src.getWidth();

  bool breaked = false;

  PixelsRect tsrc_rect = src.duplicate();
  PixelsRect *tsrc = &tsrc_rect;
  READ_DECL(tsrc);

  PixelsRect tdst_rect = dst.duplicate();
  PixelsRect *tdst = &tdst_rect;
  READ_DECL(tdst);

  WRITE_DECL(dst);

  memset(tdst_img.buffer, 0, tdst_img.brow_bytes * tdst_img.col_elems);
  memset(dst_img.buffer, 0, dst_img.brow_bytes * dst_img.col_elems);

  float coordf_x, coordf_y;
  for (a = 0.0f; a < DEG2RADF(360.0f) && (!breaked); a += ang) {
    const float an = a + settings->angle_ofs;
    const float vx = cos((double)an), vy = sin((double)an);
    for (n = 0; n < settings->iter && (!breaked); n++) {
      const float p4 = pow(4.0, (double)n);
      const float vxp = vx * p4, vyp = vy * p4;
      const float wt = pow((double)settings->fade, (double)p4);
      const float cmo = 1.0f -
                        (float)pow((double)settings->colmod,
                                   (double)n +
                                       1);  // colormodulation amount relative to current pass

      SET_COORDS(tdst, 0, 0);
      while (tdst_coords.y < src_height && (!breaked)) {
        while (tdst_coords.x < src_width) {
          // first pass no offset, always same for every pass, exact copy,
          // otherwise results in uneven brightness, only need once
          if (n == 0) {
            COPY_COORDS(tsrc, tdst_coords);
            READ_IMG4(tsrc, c1);
          }
          else {
            c1 = zero_f4;
          }

          coordf_x = tdst_coords.x + vxp;
          coordf_y = tdst_coords.y + vyp;
          SET_SAMPLE_COORDS(tsrc, coordf_x, coordf_y);
          SAMPLE_BILINEAR4_CLIP(0, tsrc, sampler, c2);

          coordf_x = tdst_coords.x + vxp * 2.0f;
          coordf_y = tdst_coords.y + vyp * 2.0f;
          SET_SAMPLE_COORDS(tsrc, coordf_x, coordf_y);
          SAMPLE_BILINEAR4_CLIP(1, tsrc, sampler, c3);

          coordf_x = tdst_coords.x + vxp * 3.0f;
          coordf_y = tdst_coords.y + vyp * 3.0f;
          SET_SAMPLE_COORDS(tsrc, coordf_x, coordf_y);
          SAMPLE_BILINEAR4_CLIP(2, tsrc, sampler, c4);

          // modulate color to look vaguely similar to a color spectrum
          c2.y *= cmo;
          c2.z *= cmo;

          c3.x *= cmo;
          c3.y *= cmo;

          c4.x *= cmo;
          c4.z *= cmo;

          READ_IMG4(tdst, tdst_pix);

          tdst_pix = half_f4 * (tdst_pix + c1 + wt * (c2 + wt * (c3 + wt * c4)));
          tdst_pix.w = 1.0f;
          WRITE_IMG4(tdst, tdst_pix);

          INCR1_COORDS_X(tdst);
        }
        INCR1_COORDS_Y(tdst);
        UPDATE_COORDS_X(tdst, 0);
        if (isBreaked()) {
          breaked = true;
        }
      }
      PixelsUtil::copyEqualRects(tsrc_rect, tdst_rect);
    }

    float factor = 1.0f / (float)(6 - settings->iter);
    CCL::float4 factor_f4 = CCL::make_float4_1(factor);
    SET_COORDS(tsrc, 0, 0);
    SET_COORDS(dst, 0, 0);
    while (dst_coords.y < src_height && (!breaked)) {
      while (dst_coords.x < src_width) {
        READ_IMG4(tsrc, c1);
        READ_IMG4(dst, result_pix);
        result_pix += c1 * factor_f4;
        result_pix.w = 1.0f;
        WRITE_IMG4(dst, result_pix);

        INCR1_COORDS_X(tsrc);
        INCR1_COORDS_X(dst);
      }
      INCR1_COORDS_Y(tsrc);
      INCR1_COORDS_Y(dst);
      UPDATE_COORDS_X(tsrc, 0);
      UPDATE_COORDS_X(dst, 0);
    }

    memset(tdst_img.buffer, 0, tdst_img.brow_bytes * tdst_img.col_elems);
    PixelsUtil::copyEqualRects(tsrc_rect, src);
    nump++;
  }

  recycler->giveRecycle(tsrc_rect.tmp_buffer);
  recycler->giveRecycle(tdst_rect.tmp_buffer);
}
