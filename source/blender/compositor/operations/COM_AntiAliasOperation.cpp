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

#include "COM_AntiAliasOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"

/* An implementation of the Scale3X edge-extrapolation algorithm.
 *
 * Code from GIMP plugin, based on code from Adam D. Moss (adam@gimp.org)
 * licensed by the MIT license.
 */
static int extrapolate9(float *E0,
                        float *E1,
                        float *E2,
                        float *E3,
                        float *E4,
                        float *E5,
                        float *E6,
                        float *E7,
                        float *E8,
                        const float *A,
                        const float *B,
                        const float *C,
                        const float *D,
                        const float *E,
                        const float *F,
                        const float *G,
                        const float *H,
                        const float *I)
{
#define PEQ(X, Y) (fabsf(*X - *Y) < 1e-3f)
#define PCPY(DST, SRC) \
  do { \
    *DST = *SRC; \
  } while (0)
  if ((!PEQ(B, H)) && (!PEQ(D, F))) {
    if (PEQ(D, B)) {
      PCPY(E0, D);
    }
    else {
      PCPY(E0, E);
    }
    if ((PEQ(D, B) && !PEQ(E, C)) || (PEQ(B, F) && !PEQ(E, A))) {
      PCPY(E1, B);
    }
    else {
      PCPY(E1, E);
    }
    if (PEQ(B, F)) {
      PCPY(E2, F);
    }
    else {
      PCPY(E2, E);
    }
    if ((PEQ(D, B) && !PEQ(E, G)) || (PEQ(D, H) && !PEQ(E, A))) {
      PCPY(E3, D);
    }
    else {
      PCPY(E3, E);
    }
    PCPY(E4, E);
    if ((PEQ(B, F) && !PEQ(E, I)) || (PEQ(H, F) && !PEQ(E, C))) {
      PCPY(E5, F);
    }
    else {
      PCPY(E5, E);
    }
    if (PEQ(D, H)) {
      PCPY(E6, D);
    }
    else {
      PCPY(E6, E);
    }
    if ((PEQ(D, H) && !PEQ(E, I)) || (PEQ(H, F) && !PEQ(E, G))) {
      PCPY(E7, H);
    }
    else {
      PCPY(E7, E);
    }
    if (PEQ(H, F)) {
      PCPY(E8, F);
    }
    else {
      PCPY(E8, E);
    }
    return 1;
  }
  else {
    return 0;
  }
#undef PEQ
#undef PCPY
}

AntiAliasOperation::AntiAliasOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::VALUE);
}

void AntiAliasOperation::execPixels(ExecutionManager &man)
{
  auto input = getInputOperation(0)->getPixels(this, man);
  auto cpu_write = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    int input_w = input->getWidth();
    int input_h = input->getHeight();
    READ_DECL(input);
    WRITE_DECL(dst);
    CPU_LOOP_START(dst);

    COPY_COORDS(input, dst_coords);

    if (input_coords.y < 0 || input_coords.y >= input_h || input_coords.x < 0 ||
        input_coords.x >= input_w) {
      dst_img.buffer[dst_offset] = 0.0f;
    }
    else {
      const float *row_curr = &input_img.buffer[input_coords.y * input_img.brow_chs_incr];
      if (input_coords.x == 0 || input_coords.x == input_w - 1 || input_coords.y == 0 ||
          input_coords.y == input_h - 1) {
        size_t x_offset = input_coords.x * input_img.belem_chs_incr;
        dst_img.buffer[dst_offset] = row_curr[x_offset];
      }
      else {
        const float *row_prev = &input_img.buffer[(input_coords.y - 1) * input_img.brow_chs_incr],
                    *row_next = &input_img.buffer[(input_coords.y + 1) * input_img.brow_chs_incr];
        size_t x_offset = input_coords.x * input_img.belem_chs_incr;
        float ninepix[9];
        if (extrapolate9(&ninepix[0],
                         &ninepix[1],
                         &ninepix[2],
                         &ninepix[3],
                         &ninepix[4],
                         &ninepix[5],
                         &ninepix[6],
                         &ninepix[7],
                         &ninepix[8],
                         &row_prev[x_offset - 1],
                         &row_prev[x_offset],
                         &row_prev[x_offset + 1],
                         &row_curr[x_offset - 1],
                         &row_curr[x_offset],
                         &row_curr[x_offset + 1],
                         &row_next[x_offset - 1],
                         &row_next[x_offset],
                         &row_next[x_offset + 1])) {
          /* Some rounding magic to so make weighting correct with the
           * original coefficients.
           */
          unsigned char result = ((3 * ninepix[0] + 5 * ninepix[1] + 3 * ninepix[2] +
                                   5 * ninepix[3] + 6 * ninepix[4] + 5 * ninepix[5] +
                                   3 * ninepix[6] + 5 * ninepix[7] + 3 * ninepix[8]) *
                                      255.0f +
                                  19.0f) /
                                 38.0f;
          dst_img.buffer[dst_offset] = result / 255.0f;
        }
        else {
          dst_img.buffer[dst_offset] = row_curr[x_offset];
        }
      }
    }

    CPU_LOOP_END;
  };
  cpuWriteSeek(man, cpu_write);
}
