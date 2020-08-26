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

#include "COM_GlareSimpleStarOperation.h"
#include "COM_ExecutionManager.h"
#include "COM_GlobalManager.h"
#include "COM_PixelsUtil.h"

#include "COM_kernel_cpu.h"

void GlareSimpleStarOperation::generateGlare(PixelsRect &dst,
                                             PixelsRect &src,
                                             NodeGlare *settings,
                                             ExecutionManager &man)
{
  int i, x, y, ym, yp, xm, xp;
  const float f1 = 1.0f - settings->fade;
  const float f2 = (1.0f - f1) * 0.5f;
  const CCL::float4 f1_f4 = CCL::make_float4_1(f1);
  const CCL::float4 f2_f4 = CCL::make_float4_1(f2);
  CCL::float4 c = CCL::make_float4_1(0.0f);
  CCL::float4 tc = CCL::make_float4_1(0.0f);
  BufferRecycler *recycler = GlobalMan->BufferMan->recycler();

  PixelsRect t1_rect = src.duplicate();
  PixelsRect *t1 = &t1_rect;
  READ_DECL(t1);

  PixelsRect t2_rect = src.duplicate();
  PixelsRect *t2 = &t2_rect;
  READ_DECL(t2);

  WRITE_DECL(dst);

  int width = getWidth();
  int height = getHeight();
  PixelsSampler sampler = PixelsSampler{PixelInterpolation::NEAREST, PixelExtend::CLIP};

  bool breaked = false;
  int coord_x, coord_y;
  for (i = 0; i < settings->iter && (!breaked); i++) {
    //      // (x || x-1, y-1) to (x || x+1, y+1)
    //      // F

    for (y = 0; y < height && (!breaked); y++) {
      ym = y - i;
      yp = y + i;
      for (x = 0; x < width; x++) {
        xm = x - i;
        xp = x + i;

        SET_COORDS(t1, x, y);
        READ_IMG4(t1, c);
        c *= f1_f4;

        coord_x = settings->star_45 ? xm : x;
        SET_COORDS(t1, coord_x, ym);
        READ_NEAREST4_CLIP(0, t1, sampler, tc);
        c += tc * f2_f4;

        coord_x = settings->star_45 ? xp : x;
        SET_COORDS(t1, coord_x, yp);
        READ_NEAREST4_CLIP(1, t1, sampler, tc);
        c += tc * f2_f4;

        c.w = 1.0f;

        SET_COORDS(t1, x, y);
        WRITE_IMG4(t1, c);

        COPY_COORDS(t2, t1_coords);
        READ_IMG4(t2, c);
        c *= f1_f4;

        coord_y = settings->star_45 ? yp : y;
        SET_COORDS(t2, xm, coord_y);
        READ_NEAREST4_CLIP(0, t2, sampler, tc);
        c += tc * f2_f4;

        coord_y = settings->star_45 ? ym : y;
        SET_COORDS(t2, xp, coord_y);
        READ_NEAREST4_CLIP(1, t2, sampler, tc);
        c += tc * f2_f4;

        c.w = 1.0f;

        SET_COORDS(t2, x, y);
        WRITE_IMG4(t2, c);
      }
      if (man.isBreaked()) {
        breaked = true;
      }
    }
    //      // B
    for (y = height - 1; y >= 0 && (!breaked); y--) {
      ym = y - i;
      yp = y + i;
      for (x = width - 1; x >= 0; x--) {
        xm = x - i;
        xp = x + i;

        SET_COORDS(t1, x, y);
        READ_IMG4(t1, c);
        c *= f1_f4;

        coord_x = settings->star_45 ? xm : x;
        SET_COORDS(t1, coord_x, ym);
        READ_NEAREST4_CLIP(0, t1, sampler, tc);
        c += tc * f2_f4;

        coord_x = settings->star_45 ? xp : x;
        SET_COORDS(t1, coord_x, yp);
        READ_NEAREST4_CLIP(1, t1, sampler, tc);
        c += tc * f2_f4;

        c.w = 1.0f;

        SET_COORDS(t1, x, y);
        WRITE_IMG4(t1, c);

        COPY_COORDS(t2, t1_coords);
        READ_IMG4(t2, c);
        c *= f1_f4;

        coord_y = settings->star_45 ? yp : y;
        SET_COORDS(t2, xm, coord_y);
        READ_NEAREST4_CLIP(0, t2, sampler, tc);
        c += tc * f2_f4;

        coord_y = settings->star_45 ? ym : y;
        SET_COORDS(t2, xp, coord_y);
        READ_NEAREST4_CLIP(1, t2, sampler, tc);
        c += tc * f2_f4;

        c.w = 1.0f;

        SET_COORDS(t2, x, y);
        WRITE_IMG4(t2, c);
      }
      if (man.isBreaked()) {
        breaked = true;
      }
    }
  }

  CCL::float4 result_pix;
  SET_COORDS(dst, 0, 0);
  COPY_COORDS(t1, dst_coords);
  COPY_COORDS(t2, dst_coords);
  while (dst_coords.y < height) {
    while (dst_coords.x < width) {
      READ_IMG4(t1, c);
      READ_IMG4(t2, tc);
      result_pix = c + tc;
      WRITE_IMG4(dst, result_pix);

      INCR1_COORDS_X(t1);
      INCR1_COORDS_X(t2);
      INCR1_COORDS_X(dst);
    }
    INCR1_COORDS_Y(t1);
    INCR1_COORDS_Y(t2);
    INCR1_COORDS_Y(dst);
    UPDATE_COORDS_X(t1, 0);
    UPDATE_COORDS_X(t2, 0);
    UPDATE_COORDS_X(dst, 0);
  }

  recycler->giveRecycle(t1_rect.tmp_buffer);
  recycler->giveRecycle(t2_rect.tmp_buffer);
}
