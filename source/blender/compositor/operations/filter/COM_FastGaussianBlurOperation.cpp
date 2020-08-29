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

#include <limits.h>

#include "COM_FastGaussianBlurOperation.h"
#include "COM_PixelsUtil.h"
#include "MEM_guardedalloc.h"
#include <algorithm>

FastGaussianBlurOperation::FastGaussianBlurOperation() : BlurBaseOperation(SocketType::COLOR)
{
}

void FastGaussianBlurOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  float sx = m_data.sizex * m_size / 2.0f;
  float sy = m_data.sizey * m_size / 2.0f;
  auto cpuWrite = [&, sx, sy](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    PixelsRect color_rect = color->toRect(dst);
    PixelsUtil::copyEqualRects(dst, color_rect);
    PixelsImg dst_img = dst.pixelsImg();
    if (sx == sy && sx > 0.0f) {
      for (int c = 0; c < COM_NUM_CHANNELS_COLOR; c++) {
        IIR_gauss(dst_img, sx, c, 3);
      }
    }
    else {
      if (sx > 0.0f) {
        for (int c = 0; c < COM_NUM_CHANNELS_COLOR; c++) {
          IIR_gauss(dst_img, sx, c, 1);
        }
      }
      if (sy > 0.0f) {
        for (int c = 0; c < COM_NUM_CHANNELS_COLOR; c++) {
          IIR_gauss(dst_img, sy, c, 2);
        }
      }
    }
  };
  return cpuWriteSeek(man, cpuWrite);
}

/// <summary>
/// Only works for full operation rects.
/// TODO: make it work for any rect by passing src img and dst image rect separately
/// </summary>
/// <param name="src_dst"></param>
/// <param name="sigma"></param>
/// <param name="chan"></param>
/// <param name="xy"></param>
void FastGaussianBlurOperation::IIR_gauss(PixelsImg &src_dst,
                                          float sigma,
                                          unsigned int chan,
                                          unsigned int xy)
{
  BLI_assert(!src_dst.is_single_elem);
  double q, q2, sc, cf[4], tsM[9], tsu[3], tsv[3];
  double *X, *Y, *W;
  const unsigned int src_width = src_dst.row_elems;
  const unsigned int src_height = src_dst.col_elems;
  unsigned int x, y, sz;
  unsigned int i;
  float *buffer = src_dst.buffer;
  const unsigned int num_channels = src_dst.elem_chs;

  // <0.5 not valid, though can have a possibly useful sort of sharpening effect
  if (sigma < 0.5f) {
    return;
  }

  if ((xy < 1) || (xy > 3)) {
    xy = 3;
  }

  // XXX The YVV macro defined below explicitly expects sources of at least 3x3 pixels,
  //     so just skipping blur along faulty direction if src's def is below that limit!
  if (src_width < 3) {
    xy &= ~1;
  }
  if (src_height < 3) {
    xy &= ~2;
  }
  if (xy < 1) {
    return;
  }

  // see "Recursive Gabor Filtering" by Young/VanVliet
  // all factors here in double.prec.
  // Required, because for single.prec it seems to blow up if sigma > ~200
  if (sigma >= 3.556f) {
    q = 0.9804 * (sigma - 3.556) + 2.5091;
  }
  else {  // sigma >= 0.5
    q = (0.0561 * sigma + 0.5784) * sigma - 0.2568;
  }
  q2 = q * q;
  sc = (1.1668 + q) * (3.203729649 + (2.21566 + q) * q);
  // no gabor filtering here, so no complex multiplies, just the regular coefs.
  // all negated here, so as not to have to recalc Triggs/Sdika matrix
  cf[1] = q * (5.788961737 + (6.76492 + 3.0 * q) * q) / sc;
  cf[2] = -q2 * (3.38246 + 3.0 * q) / sc;
  // 0 & 3 unchanged
  cf[3] = q2 * q / sc;
  cf[0] = 1.0 - cf[1] - cf[2] - cf[3];

  // Triggs/Sdika border corrections,
  // it seems to work, not entirely sure if it is actually totally correct,
  // Besides J.M.Geusebroek's anigauss.c (see http://www.science.uva.nl/~mark),
  // found one other implementation by Cristoph Lampert,
  // but neither seem to be quite the same, result seems to be ok so far anyway.
  // Extra scale factor here to not have to do it in filter,
  // though maybe this had something to with the precision errors
  sc = cf[0] / ((1.0 + cf[1] - cf[2] + cf[3]) * (1.0 - cf[1] - cf[2] - cf[3]) *
                (1.0 + cf[2] + (cf[1] - cf[3]) * cf[3]));
  tsM[0] = sc * (-cf[3] * cf[1] + 1.0 - cf[3] * cf[3] - cf[2]);
  tsM[1] = sc * ((cf[3] + cf[1]) * (cf[2] + cf[3] * cf[1]));
  tsM[2] = sc * (cf[3] * (cf[1] + cf[3] * cf[2]));
  tsM[3] = sc * (cf[1] + cf[3] * cf[2]);
  tsM[4] = sc * (-(cf[2] - 1.0) * (cf[2] + cf[3] * cf[1]));
  tsM[5] = sc * (-(cf[3] * cf[1] + cf[3] * cf[3] + cf[2] - 1.0) * cf[3]);
  tsM[6] = sc * (cf[3] * cf[1] + cf[2] + cf[1] * cf[1] - cf[2] * cf[2]);
  tsM[7] = sc * (cf[1] * cf[2] + cf[3] * cf[2] * cf[2] - cf[1] * cf[3] * cf[3] -
                 cf[3] * cf[3] * cf[3] - cf[3] * cf[2] + cf[3]);
  tsM[8] = sc * (cf[3] * (cf[1] + cf[3] * cf[2]));

#define YVV(L) \
  { \
    W[0] = cf[0] * X[0] + cf[1] * X[0] + cf[2] * X[0] + cf[3] * X[0]; \
    W[1] = cf[0] * X[1] + cf[1] * W[0] + cf[2] * X[0] + cf[3] * X[0]; \
    W[2] = cf[0] * X[2] + cf[1] * W[1] + cf[2] * W[0] + cf[3] * X[0]; \
    for (i = 3; i < L; i++) { \
      W[i] = cf[0] * X[i] + cf[1] * W[i - 1] + cf[2] * W[i - 2] + cf[3] * W[i - 3]; \
    } \
    tsu[0] = W[L - 1] - X[L - 1]; \
    tsu[1] = W[L - 2] - X[L - 1]; \
    tsu[2] = W[L - 3] - X[L - 1]; \
    tsv[0] = tsM[0] * tsu[0] + tsM[1] * tsu[1] + tsM[2] * tsu[2] + X[L - 1]; \
    tsv[1] = tsM[3] * tsu[0] + tsM[4] * tsu[1] + tsM[5] * tsu[2] + X[L - 1]; \
    tsv[2] = tsM[6] * tsu[0] + tsM[7] * tsu[1] + tsM[8] * tsu[2] + X[L - 1]; \
    Y[L - 1] = cf[0] * W[L - 1] + cf[1] * tsv[0] + cf[2] * tsv[1] + cf[3] * tsv[2]; \
    Y[L - 2] = cf[0] * W[L - 2] + cf[1] * Y[L - 1] + cf[2] * tsv[0] + cf[3] * tsv[1]; \
    Y[L - 3] = cf[0] * W[L - 3] + cf[1] * Y[L - 2] + cf[2] * Y[L - 1] + cf[3] * tsv[0]; \
    /* 'i != UINT_MAX' is really 'i >= 0', but necessary for unsigned int wrapping */ \
    for (i = L - 4; i != UINT_MAX; i--) { \
      Y[i] = cf[0] * W[i] + cf[1] * Y[i + 1] + cf[2] * Y[i + 2] + cf[3] * Y[i + 3]; \
    } \
  } \
  (void)0

  // intermediate buffers
  sz = std::max(src_width, src_height);
  X = (double *)MEM_callocN(sz * sizeof(double), "IIR_gauss X buf");
  Y = (double *)MEM_callocN(sz * sizeof(double), "IIR_gauss Y buf");
  W = (double *)MEM_callocN(sz * sizeof(double), "IIR_gauss W buf");
  if (xy & 1) {  // H
    int offset;
    for (y = 0; y < src_height; y++) {
      const int yx = y * src_width;
      offset = yx * num_channels + chan;
      for (x = 0; x < src_width; x++) {
        X[x] = buffer[offset];
        offset += num_channels;
      }
      YVV(src_width);
      offset = yx * num_channels + chan;
      for (x = 0; x < src_width; x++) {
        buffer[offset] = Y[x];
        offset += num_channels;
      }
    }
  }
  if (xy & 2) {  // V
    int offset;
    const int add = src_width * num_channels;

    for (x = 0; x < src_width; x++) {
      offset = x * num_channels + chan;
      for (y = 0; y < src_height; y++) {
        X[y] = buffer[offset];
        offset += add;
      }
      YVV(src_height);
      offset = x * num_channels + chan;
      for (y = 0; y < src_height; y++) {
        buffer[offset] = Y[y];
        offset += add;
      }
    }
  }

  MEM_freeN(X);
  MEM_freeN(W);
  MEM_freeN(Y);
#undef YVV
}

FastGaussianBlurValueOperation::FastGaussianBlurValueOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::VALUE);
  this->m_sigma = 1.0f;
  this->m_overlay = 0;
}

void FastGaussianBlurValueOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_sigma);
  hashParam(m_overlay);
}

void FastGaussianBlurValueOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);

  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    auto color_rect = color->toRect(dst);
    PixelsUtil::copyEqualRects(dst, color_rect);
    PixelsImg dst_img = dst.pixelsImg();
    FastGaussianBlurOperation::IIR_gauss(dst_img, m_sigma, 0, 3);

    PixelsImg src_img = color->pixelsImg();

    if (this->m_overlay == FAST_GAUSS_OVERLAY_MIN) {
      float *dst_cur = dst_img.start;
      float *src_cur = src_img.start;
      float *dst_row_end = dst_img.start + dst_img.row_chs;
      while (dst_cur < dst_img.end) {
        while (dst_cur < dst_row_end) {
          if (*src_cur < *dst_cur) {
            *dst_cur = *src_cur;
          }
          dst_cur += dst_img.elem_chs_incr;
          src_cur += src_img.elem_chs_incr;
        }
        dst_cur += dst_img.row_jump;
        src_cur += src_img.row_jump;
        dst_row_end += dst_img.brow_chs_incr;
      }
    }
    else if (this->m_overlay == FAST_GAUSS_OVERLAY_MAX) {
      float *dst_cur = dst_img.start;
      float *src_cur = src_img.start;
      float *dst_row_end = dst_img.start + dst_img.row_chs;
      while (dst_cur < dst_img.end) {
        while (dst_cur < dst_row_end) {
          if (*src_cur > *dst_cur) {
            *dst_cur = *src_cur;
          }
          dst_cur += dst_img.elem_chs_incr;
          src_cur += src_img.elem_chs_incr;
        }
        dst_cur += dst_img.row_jump;
        src_cur += src_img.row_jump;
        dst_row_end += dst_img.brow_chs_incr;
      }
    }
  };
  return cpuWriteSeek(man, cpuWrite);
}
