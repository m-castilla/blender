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

#include "COM_GlareGhostOperation.h"
#include "COM_ExecutionManager.h"
#include "COM_FastGaussianBlurOperation.h"
#include "COM_GlobalManager.h"
#include "COM_PixelsUtil.h"

#include "COM_kernel_cpu.h"

static __forceinline float smoothMask(float x, float y)
{
  float t;
  x = 2.0f * x - 1.0f;
  y = 2.0f * y - 1.0f;
  if ((t = 1.0f - sqrtf(x * x + y * y)) > 0.0f) {
    return t;
  }
  else {
    return 0.0f;
  }
}

void GlareGhostOperation::generateGlare(PixelsRect &dst,
                                        PixelsRect &src,
                                        NodeGlare *settings,
                                        ExecutionManager &man)
{
  PixelsSampler sampler = {PixelInterpolation::BILINEAR, PixelExtend::CLIP};
  const int qt = 1 << settings->quality;
  const float s1 = 4.0f / (float)qt, s2 = 2.0f * s1;
  int n, p, np;
  CCL::float4 cm[64];
  float sc, isc, u, v, sm, s, t, ofs, scalef[64] = {};
  const float cmo = 1.0f - settings->colmod;
  CCL::float4 zero_f4 = CCL::make_float4_1(0.0f);
  PixelsRect g_rect = src.duplicate();
  int g_height = g_rect.getHeight();
  int g_width = g_rect.getWidth();
  PixelsRect *g = &g_rect;
  READ_DECL(g);

  PixelsRect t1_rect = src.duplicate();
  PixelsRect *t1 = &t1_rect;
  READ_DECL(t1);

  if (!man.isBreaked()) {
    FastGaussianBlurOperation::IIR_gauss(t1_img, s1, 0, 3);
    FastGaussianBlurOperation::IIR_gauss(t1_img, s1, 1, 3);
    FastGaussianBlurOperation::IIR_gauss(t1_img, s1, 2, 3);
  }

  PixelsRect t2_rect = t1_rect.duplicate();
  PixelsRect *t2 = &t2_rect;
  READ_DECL(t2);

  if (!man.isBreaked()) {
    FastGaussianBlurOperation::IIR_gauss(t2_img, s2, 0, 3);
    FastGaussianBlurOperation::IIR_gauss(t2_img, s2, 1, 3);
    FastGaussianBlurOperation::IIR_gauss(t2_img, s2, 2, 3);
  }

  if (!man.isBreaked()) {
    ofs = (settings->iter & 1) ? 0.5f : 0.0f;
    CCL::float4 cmo1 = CCL::make_float4(1.0f, cmo, cmo, 1.0f);
    CCL::float4 cmo2 = CCL::make_float4(cmo, 1.0f, cmo, 1.0f);
    CCL::float4 cmo3 = CCL::make_float4(cmo, cmo, 1.0f, 1.0f);
    for (int x = 0, y = 0; x < (settings->iter * 4); x++) {
      y = x & 3;
      cm[x] = CCL::make_float4_1(1.0f);
      if (y == 1) {
        cm[x] *= cmo1;
      }
      if (y == 2) {
        cm[x] *= cmo3;
      }
      if (y == 3) {
        cm[x] *= cmo2;
      }
      scalef[x] = 2.1f * (1.0f - (x + ofs) / (float)(settings->iter * 4));
      if (x & 1) {
        scalef[x] = -0.99f / scalef[x];
      }
    }

    sc = 2.13;
    isc = -0.97;
    bool breaked = false;

    SET_COORDS(g, 0, 0);
    int t1_x, t1_y, t2_x, t2_y;
    while (g_coords.y < g_height && !breaked) {
      v = ((float)g_coords.y + 0.5f) / (float)g_height;
      while (g_coords.x < g_width) {
        u = ((float)g_coords.x + 0.5f) / (float)g_width;
        s = (u - 0.5f) * sc + 0.5f;
        t = (v - 0.5f) * sc + 0.5f;
        t1_x = s * g_width;
        t1_y = t * g_height;
        SET_SAMPLE_COORDS(t1, t1_x, t1_y);
        SAMPLE_BILINEAR4_CLIP(0, t1, sampler, t1_pix);
        sm = smoothMask(s, t);
        t1_pix *= sm;
        s = (u - 0.5f) * isc + 0.5f;
        t = (v - 0.5f) * isc + 0.5f;
        t2_x = s * g_width - 0.5f;
        t2_y = t * g_height - 0.5f;
        SET_SAMPLE_COORDS(t2, t2_x, t2_y);
        SAMPLE_BILINEAR4_CLIP(0, t2, sampler, t2_pix);
        sm = smoothMask(s, t);
        t1_pix += t2_pix * sm;

        WRITE_IMG4(g, t1_pix);
        INCR1_COORDS_X(g);
      }
      breaked = man.isBreaked();
      INCR1_COORDS_Y(g);
      UPDATE_COORDS_X(g, 0);
    }
  }

  if (!man.isBreaked()) {
    memset(t1_img.buffer, 0, t1_img.brow_bytes * t1_img.col_elems);
    bool breaked = false;
    int g_x, g_y;
    for (n = 1; n < settings->iter && (!breaked); n++) {
      SET_COORDS(t1, 0, 0);
      while (t1_coords.y < g_height && (!breaked)) {
        v = ((float)t1_coords.y + 0.5f) / (float)g_height;
        while (t1_coords.x < g_width) {
          u = ((float)t1_coords.x + 0.5f) / (float)g_width;
          t2_pix = zero_f4;
          for (p = 0; p < 4; p++) {
            np = (n << 2) + p;
            s = (u - 0.5f) * scalef[np] + 0.5f;
            t = (v - 0.5f) * scalef[np] + 0.5f;
            g_x = s * g_width - 0.5f;
            g_y = t * g_height - 0.5f;
            SET_SAMPLE_COORDS(g, g_x, g_y);
            SAMPLE_BILINEAR4_CLIP(0, g, sampler, g_pix);
            g_pix *= cm[np];
            sm = smoothMask(s, t) * 0.25f;
            t2_pix += g_pix * sm;
          }
          READ_IMG4(t1, t1_pix);
          t2_pix += t1_pix;
          t2_pix.w = t1_pix.w;
          WRITE_IMG4(t1, t2_pix);
          INCR1_COORDS_X(t1);
        }
        breaked = man.isBreaked();
        INCR1_COORDS_Y(t1);
        UPDATE_COORDS_X(t1, 0);
      }
      PixelsUtil::copyEqualRects(g_rect, t1_rect);
    }
    PixelsUtil::copyEqualRects(dst, g_rect);
  }

  auto recycler = GlobalMan->BufferMan->recycler();
  recycler->giveRecycle(g_rect.tmp_buffer);
  recycler->giveRecycle(t1_rect.tmp_buffer);
  recycler->giveRecycle(t2_rect.tmp_buffer);
}
