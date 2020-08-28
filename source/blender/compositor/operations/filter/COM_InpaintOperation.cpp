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

#include "MEM_guardedalloc.h"

#include "COM_InpaintOperation.h"
#include "COM_PixelsUtil.h"
#include "COM_kernel_cpu.h"

#define ASSERT_XY_RANGE(x, y, width, height) \
  BLI_assert(x >= 0 && x < width && y >= 0 && y < height)

static __forceinline void clamp_xy(int &x, int &y, int width, int height)
{
  if (x < 0) {
    x = 0;
  }
  else if (x >= width) {
    x = width - 1;
  }

  if (y < 0) {
    y = 0;
  }
  else if (y >= height) {
    y = height - 1;
  }
}

// Inpaint (simple convolve using average of known pixels)
InpaintSimpleOperation::InpaintSimpleOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
  m_iterations = 0;
}
void InpaintSimpleOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_iterations);
}

void InpaintSimpleOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto cpu_write = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    int width = this->getWidth();
    int height = this->getHeight();

    short *manhattan_distance = (short *)MEM_mallocN(sizeof(short) * width * height, __func__);
    int *offsets = (int *)MEM_callocN(sizeof(int) * ((size_t)width + height + 1),
                                      "InpaintSimpleOperation offsets");

    // Copy color to dst, there will be several passes over dst and we must start with color pixels
    PixelsUtil::copyEqualRects(dst, *color);

    CCL::float4 color_pix;
    WRITE_DECL(dst);

    SET_COORDS(dst, 0, 0);
    while (dst_coords.y < height) {
      int y_offset = dst_coords.y * width;
      int y_offset_prev = (dst_coords.y - 1) * width;
      while (dst_coords.x < width) {
        int r = 0;
        float alpha = dst_img.buffer[dst_offset + 3];
        if (alpha < 1.0f) {
          r = width + height;
          if (dst_coords.x > 0) {
            r = CCL::min(r, manhattan_distance[y_offset + dst_coords.x - 1] + 1);
          }
          if (dst_coords.y > 0) {
            r = CCL::min(r, manhattan_distance[y_offset_prev + dst_coords.x] + 1);
          }
        }
        manhattan_distance[y_offset + dst_coords.x] = r;
        INCR1_COORDS_X(dst);
      }
      INCR1_COORDS_Y(dst);
      UPDATE_COORDS_X(dst, 0);
    }

    for (int j = height - 1; j >= 0; j--) {
      int y_offset = j * width;
      int y_offset_prev = (j + 1) * width;
      for (int i = width - 1; i >= 0; i--) {
        int r = manhattan_distance[j * width + i];

        if (i + 1 < width) {
          r = CCL::min(r, manhattan_distance[y_offset + i + 1] + 1);
        }
        if (j + 1 < height) {
          r = CCL::min(r, manhattan_distance[y_offset_prev + i] + 1);
        }

        manhattan_distance[y_offset + i] = r;

        offsets[r]++;
      }
    }

    offsets[0] = 0;

    for (int i = 1; i < width + height + 1; i++) {
      offsets[i] += offsets[i - 1];
    }

    int area_size = offsets[width + height];
    int *pixelorder = (int *)MEM_mallocN(sizeof(int) * area_size, __func__);

    for (int i = 0; i < width * height; i++) {
      if (manhattan_distance[i] > 0) {
        pixelorder[offsets[manhattan_distance[i] - 1]++] = i;
      }
    }

    MEM_freeN(offsets);
    offsets = nullptr;

    CCL::float4 zero_f4 = CCL::make_float4_1(0.0f);
    CCL::float4 one_f4 = CCL::make_float4_1(1.0f);
    CCL::float4 sqrt1_2_f4 = CCL::make_float4_1(M_SQRT1_2_F); /* 1.0f / sqrt(2) */
    CCL::float4 weight_f4;

    int curr = 0;
    int x, y;
    while (curr < area_size) {
      int r = pixelorder[curr];

      x = r % width;
      y = r / width;

      ASSERT_XY_RANGE(x, y, width, height);
      const int distance = manhattan_distance[y * width + x];
      if (distance > m_iterations) {
        break;
      }
      else {
        curr++;
      }

      CCL::float4 pix_accum = zero_f4;
      float pix_divider = 0.0f;
      for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
          /* changing to both != 0 gives dithering artifacts */
          if (dx != 0 || dy != 0) {
            int x_ofs = x + dx;
            int y_ofs = y + dy;

            clamp_xy(x_ofs, y_ofs, width, height);
            ASSERT_XY_RANGE(x_ofs, y_ofs, width, height);
            if (manhattan_distance[y_ofs * width + x_ofs] < distance) {
              if (dx == 0 || dy == 0) {
                weight_f4 = one_f4;
              }
              else {
                weight_f4 = sqrt1_2_f4; /* 1.0f / sqrt(2) */
              }

              SET_COORDS(dst, x_ofs, y_ofs);
              READ_IMG4(dst, color_pix);
              pix_accum += color_pix * weight_f4;
              pix_divider += weight_f4.x;
            }
          }
        }
      }

      if (pix_divider != 0.0f) {
        pix_accum *= 1.0f / pix_divider;

        SET_COORDS(dst, x, y);
        READ_IMG4(dst, color_pix);

        /* use existing pixels alpha to blend into */
        pix_accum = CCL::interp_f4f4(pix_accum, color_pix, color_pix.w);
        pix_accum.w = 1.0f;
        WRITE_IMG4(dst, pix_accum);
      }
    }

    MEM_freeN(manhattan_distance);
  };
  cpuWriteSeek(man, cpu_write);
}
