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

#include "COM_GlareFogGlowOperation.h"
#include "BLI_math.h"
#include "COM_GlobalManager.h"
#include "COM_PixelsUtil.h"
#include "MEM_guardedalloc.h"

#include "COM_kernel_cpu.h"

#include <math.h>

/*
 *  2D Fast Hartley Transform, used for convolution
 */

typedef float fREAL;

// returns next highest power of 2 of x, as well it's log2 in L2
static unsigned int nextPow2(unsigned int x, unsigned int *L2)
{
  unsigned int pw, x_notpow2 = x & (x - 1);
  *L2 = 0;
  while (x >>= 1) {
    ++(*L2);
  }
  pw = 1 << (*L2);
  if (x_notpow2) {
    (*L2)++;
    pw <<= 1;
  }
  return pw;
}

//------------------------------------------------------------------------------

// from FXT library by Joerg Arndt, faster in order bitreversal
// use: r = revbin_upd(r, h) where h = N>>1
static unsigned int revbin_upd(unsigned int r, unsigned int h)
{
  while (!((r ^= h) & h)) {
    h >>= 1;
  }
  return r;
}
//------------------------------------------------------------------------------
static void FHT(fREAL *data, unsigned int M, unsigned int inverse)
{
  double tt, fc, dc, fs, ds, a = M_PI;
  fREAL t1, t2;
  int n2, bd, bl, istep, k, len = 1 << M, n = 1;

  int i, j = 0;
  unsigned int Nh = len >> 1;
  for (i = 1; i < (len - 1); i++) {
    j = revbin_upd(j, Nh);
    if (j > i) {
      t1 = data[i];
      data[i] = data[j];
      data[j] = t1;
    }
  }

  do {
    fREAL *data_n = &data[n];

    istep = n << 1;
    for (k = 0; k < len; k += istep) {
      t1 = data_n[k];
      data_n[k] = data[k] - t1;
      data[k] += t1;
    }

    n2 = n >> 1;
    if (n > 2) {
      fc = dc = cos(a);
      fs = ds = sqrt(1.0 - fc * fc);  // sin(a);
      bd = n - 2;
      for (bl = 1; bl < n2; bl++) {
        fREAL *data_nbd = &data_n[bd];
        fREAL *data_bd = &data[bd];
        for (k = bl; k < len; k += istep) {
          t1 = fc * (double)data_n[k] + fs * (double)data_nbd[k];
          t2 = fs * (double)data_n[k] - fc * (double)data_nbd[k];
          data_n[k] = data[k] - t1;
          data_nbd[k] = data_bd[k] - t2;
          data[k] += t1;
          data_bd[k] += t2;
        }
        tt = fc * dc - fs * ds;
        fs = fs * dc + fc * ds;
        fc = tt;
        bd -= 2;
      }
    }

    if (n > 1) {
      for (k = n2; k < len; k += istep) {
        t1 = data_n[k];
        data_n[k] = data[k] - t1;
        data[k] += t1;
      }
    }

    n = istep;
    a *= 0.5;
  } while (n < len);

  if (inverse) {
    fREAL sc = (fREAL)1 / (fREAL)len;
    for (k = 0; k < len; k++) {
      data[k] *= sc;
    }
  }
}
//------------------------------------------------------------------------------
/* 2D Fast Hartley Transform, Mx/My -> log2 of width/height,
 * nzp -> the row where zero pad data starts,
 * inverse -> see above */
static void FHT2D(
    fREAL *data, unsigned int Mx, unsigned int My, unsigned int nzp, unsigned int inverse)
{
  unsigned int i, j, Nx, Ny, maxy;

  Nx = 1 << Mx;
  Ny = 1 << My;

  // rows (forward transform skips 0 pad data)
  maxy = inverse ? Ny : nzp;
  for (j = 0; j < maxy; j++) {
    FHT(&data[Nx * j], Mx, inverse);
  }

  // transpose data
  if (Nx == Ny) {  // square
    for (j = 0; j < Ny; j++) {
      for (i = j + 1; i < Nx; i++) {
        unsigned int op = i + (j << Mx), np = j + (i << My);
        SWAP(fREAL, data[op], data[np]);
      }
    }
  }
  else {  // rectangular
    unsigned int k, Nym = Ny - 1, stm = 1 << (Mx + My);
    for (i = 0; stm > 0; i++) {
#define PRED(k) (((k & Nym) << Mx) + (k >> My))
      for (j = PRED(i); j > i; j = PRED(j)) {
        /* pass */
      }
      if (j < i) {
        continue;
      }
      for (k = i, j = PRED(i); j != i; k = j, j = PRED(j), stm--) {
        SWAP(fREAL, data[j], data[k]);
      }
#undef PRED
      stm--;
    }
  }

  SWAP(unsigned int, Nx, Ny);
  SWAP(unsigned int, Mx, My);

  // now columns == transposed rows
  for (j = 0; j < Ny; j++) {
    FHT(&data[Nx * j], Mx, inverse);
  }

  // finalize
  for (j = 0; j <= (Ny >> 1); j++) {
    unsigned int jm = (Ny - j) & (Ny - 1);
    unsigned int ji = j << Mx;
    unsigned int jmi = jm << Mx;
    for (i = 0; i <= (Nx >> 1); i++) {
      unsigned int im = (Nx - i) & (Nx - 1);
      fREAL A = data[ji + i];
      fREAL B = data[jmi + i];
      fREAL C = data[ji + im];
      fREAL D = data[jmi + im];
      fREAL E = (fREAL)0.5 * ((A + D) - (B + C));
      data[ji + i] = A - E;
      data[jmi + i] = B + E;
      data[ji + im] = C + E;
      data[jmi + im] = D - E;
    }
  }
}

//------------------------------------------------------------------------------

/* 2D convolution calc, d1 *= d2, M/N - > log2 of width/height */
static void fht_convolve(fREAL *d1, const fREAL *d2, unsigned int M, unsigned int N)
{
  fREAL a, b;
  unsigned int i, j, k, L, mj, mL;
  unsigned int m = 1 << M, n = 1 << N;
  unsigned int m2 = 1 << (M - 1), n2 = 1 << (N - 1);
  unsigned int mn2 = m << (N - 1);

  d1[0] *= d2[0];
  d1[mn2] *= d2[mn2];
  d1[m2] *= d2[m2];
  d1[m2 + mn2] *= d2[m2 + mn2];
  for (i = 1; i < m2; i++) {
    k = m - i;
    a = d1[i] * d2[i] - d1[k] * d2[k];
    b = d1[k] * d2[i] + d1[i] * d2[k];
    d1[i] = (b + a) * (fREAL)0.5;
    d1[k] = (b - a) * (fREAL)0.5;
    a = d1[i + mn2] * d2[i + mn2] - d1[k + mn2] * d2[k + mn2];
    b = d1[k + mn2] * d2[i + mn2] + d1[i + mn2] * d2[k + mn2];
    d1[i + mn2] = (b + a) * (fREAL)0.5;
    d1[k + mn2] = (b - a) * (fREAL)0.5;
  }
  for (j = 1; j < n2; j++) {
    L = n - j;
    mj = j << M;
    mL = L << M;
    a = d1[mj] * d2[mj] - d1[mL] * d2[mL];
    b = d1[mL] * d2[mj] + d1[mj] * d2[mL];
    d1[mj] = (b + a) * (fREAL)0.5;
    d1[mL] = (b - a) * (fREAL)0.5;
    a = d1[m2 + mj] * d2[m2 + mj] - d1[m2 + mL] * d2[m2 + mL];
    b = d1[m2 + mL] * d2[m2 + mj] + d1[m2 + mj] * d2[m2 + mL];
    d1[m2 + mj] = (b + a) * (fREAL)0.5;
    d1[m2 + mL] = (b - a) * (fREAL)0.5;
  }
  for (i = 1; i < m2; i++) {
    k = m - i;
    for (j = 1; j < n2; j++) {
      L = n - j;
      mj = j << M;
      mL = L << M;
      a = d1[i + mj] * d2[i + mj] - d1[k + mL] * d2[k + mL];
      b = d1[k + mL] * d2[i + mj] + d1[i + mj] * d2[k + mL];
      d1[i + mj] = (b + a) * (fREAL)0.5;
      d1[k + mL] = (b - a) * (fREAL)0.5;
      a = d1[i + mL] * d2[i + mL] - d1[k + mj] * d2[k + mj];
      b = d1[k + mj] * d2[i + mL] + d1[i + mL] * d2[k + mj];
      d1[i + mL] = (b + a) * (fREAL)0.5;
      d1[k + mj] = (b - a) * (fREAL)0.5;
    }
  }
}
//------------------------------------------------------------------------------

static void convolve(PixelsRect &dst, PixelsRect &src, PixelsRect &ckrn, ExecutionManager &man)
{
  fREAL *data1, *data2, *fp;
  unsigned int w2, h2, hw, hh, log2_w, log2_h;
  CCL::float4 wt, pix;
  int x, y, ch;
  int xbl, ybl, nxb, nyb, xbsz, ybsz;
  bool in2done = false;
  const unsigned int kernelWidth = ckrn.getWidth();
  const unsigned int kernelHeight = ckrn.getHeight();
  const unsigned int imageWidth = src.getWidth();
  const unsigned int imageHeight = src.getHeight();
  BufferRecycler *recycler = GlobalMan->BufferMan->recycler();

  PixelsRect *kernel = &ckrn;
  READ_DECL(kernel);
  PixelsImg src_img = src.pixelsImg();
  float *kernel_buf = kernel_img.buffer;
  float *src_buf = src_img.buffer;
  float *colp;

  TmpBuffer *rdst_tmp = recycler->createTmpBuffer(true);
  recycler->takeRecycle(
      BufferRecycleType::HOST_CLEAR, rdst_tmp, src.getWidth(), src.getHeight(), src.getElemChs());
  PixelsRect rdst_rect = PixelsRect(rdst_tmp, 0, src.getWidth(), 0, src.getHeight());
  PixelsImg rdst_img = rdst_rect.pixelsImg();
  memset(rdst_img.buffer, 0, rdst_img.brow_bytes * rdst_img.col_elems);
  float *rdst_buf = rdst_tmp->host.buffer;

  // convolution result width & height
  w2 = 2 * kernelWidth - 1;
  h2 = 2 * kernelHeight - 1;
  // FFT pow2 required size & log2
  w2 = nextPow2(w2, &log2_w);
  h2 = nextPow2(h2, &log2_h);

  // alloc space
  size_t data1_n_elems = (size_t)3 * w2 * h2;
  size_t data2_n_elems = (size_t)w2 * h2;
  TmpBuffer *data1_tmp = recycler->createTmpBuffer(true);
  recycler->takeRecycle(BufferRecycleType::HOST_CLEAR, data1_tmp, data1_n_elems, 1, 1);
  TmpBuffer *data2_tmp = recycler->createTmpBuffer(true);
  recycler->takeRecycle(BufferRecycleType::HOST_CLEAR, data2_tmp, data2_n_elems, 1, 1);

  // if someday fReal is changed to double, instead of using takeRecycle which returns float
  // buffers, we should create manually buffers of type double
  BLI_assert(sizeof(float) == sizeof(fREAL));
  data1 = (fREAL *)data1_tmp->host.buffer;
  data2 = (fREAL *)data2_tmp->host.buffer;
  memset(data1, 0, data1_n_elems * sizeof(fREAL));
  memset(data2, 0, data2_n_elems * sizeof(fREAL));
  float alpha;

  // normalize convolutor
  wt = CCL::make_float4_1(0.0f);
  SET_COORDS(kernel, 0, 0);
  while (kernel_coords.y < kernelHeight) {
    while (kernel_coords.x < kernelWidth) {
      READ_IMG4(kernel, pix);
      wt += pix;
      INCR1_COORDS_X(kernel);
    }
    INCR1_COORDS_Y(kernel);
    UPDATE_COORDS_X(kernel, 0);
  }
  if (wt.x != 0.0f) {
    wt.x = 1.0f / wt.x;
  }
  if (wt.y != 0.0f) {
    wt.y = 1.0f / wt.y;
  }
  if (wt.z != 0.0f) {
    wt.z = 1.0f / wt.z;
  }

  SET_COORDS(kernel, 0, 0);
  while (kernel_coords.y < kernelHeight) {
    while (kernel_coords.x < kernelWidth) {
      READ_IMG4(kernel, pix);
      alpha = pix.w;
      pix *= wt;
      pix.w = alpha;
      WRITE_IMG4(kernel, pix);
      INCR1_COORDS_X(kernel);
    }
    INCR1_COORDS_Y(kernel);
    UPDATE_COORDS_X(kernel, 0);
  }

  // copy image data, unpacking interleaved RGBA into separate channels
  // only need to calc data1 once

  // block add-overlap
  hw = kernelWidth >> 1;
  hh = kernelHeight >> 1;
  xbsz = (w2 + 1) - kernelWidth;
  ybsz = (h2 + 1) - kernelHeight;
  nxb = imageWidth / xbsz;
  if (imageWidth % xbsz) {
    nxb++;
  }
  nyb = imageHeight / ybsz;
  if (imageHeight % ybsz) {
    nyb++;
  }
  for (ybl = 0; ybl < nyb; ybl++) {
    for (xbl = 0; xbl < nxb; xbl++) {

      // each channel one by one
      for (ch = 0; ch < 3; ch++) {
        fREAL *data1ch = &data1[ch * w2 * h2];

        // only need to calc fht data from in2 once, can re-use for every block
        if (!in2done) {
          // in2, channel ch -> data1
          for (y = 0; y < kernelHeight; y++) {
            fp = &data1ch[y * w2];
            colp = &kernel_buf[y * kernelWidth * COM_NUM_CHANNELS_COLOR];
            for (x = 0; x < kernelWidth; x++) {
              fp[x] = colp[x * COM_NUM_CHANNELS_COLOR + ch];
            }
          }
        }

        // in1, channel ch -> data2
        memset(data2, 0, (size_t)w2 * h2 * sizeof(fREAL));
        for (y = 0; y < ybsz; y++) {
          int yy = ybl * ybsz + y;
          if (yy >= imageHeight) {
            continue;
          }
          fp = &data2[y * w2];
          colp = &src_buf[(size_t)yy * imageWidth * src_img.elem_chs_incr];
          for (x = 0; x < xbsz; x++) {
            int xx = xbl * xbsz + x;
            if (xx >= imageWidth) {
              continue;
            }
            fp[x] = colp[xx * src_img.elem_chs_incr + ch];
          }
        }

        // forward FHT
        // zero pad data start is different for each == height+1
        if (!in2done) {
          FHT2D(data1ch, log2_w, log2_h, kernelHeight + 1, 0);
        }
        FHT2D(data2, log2_w, log2_h, kernelHeight + 1, 0);

        // FHT2D transposed data, row/col now swapped
        // convolve & inverse FHT
        fht_convolve(data2, data1ch, log2_h, log2_w);
        FHT2D(data2, log2_h, log2_w, 0, 1);
        // data again transposed, so in order again

        // overlap-add result
        for (y = 0; y < (int)h2; y++) {
          const int yy = ybl * ybsz + y - hh;
          if ((yy < 0) || (yy >= imageHeight)) {
            continue;
          }
          fp = &data2[y * w2];
          colp = &rdst_buf[yy * imageWidth * COM_NUM_CHANNELS_COLOR];
          for (x = 0; x < (int)w2; x++) {
            const int xx = xbl * xbsz + x - hw;
            if ((xx < 0) || (xx >= imageWidth)) {
              continue;
            }
            colp[xx * COM_NUM_CHANNELS_COLOR + ch] += fp[x];
          }
        }
      }
      in2done = true;
    }
  }

  PixelsUtil::copyEqualRects(dst, rdst_rect);
  recycler->giveRecycle(rdst_tmp);
  recycler->giveRecycle(data1_tmp);
  recycler->giveRecycle(data2_tmp);
}

void GlareFogGlowOperation::generateGlare(PixelsRect &dst,
                                          PixelsRect &src,
                                          NodeGlare *settings,
                                          ExecutionManager &man)
{
  float scale, u, v, r, w, d;
  CCL::float4 fcol;
  unsigned int sz = 1 << settings->size;
  BufferRecycler *recycler = GlobalMan->BufferMan->recycler();

  // temp. src image
  // make the convolution kernel
  rcti kernelRect;
  BLI_rcti_init(&kernelRect, 0, sz, 0, sz);
  TmpBuffer *ckrn_buf = recycler->createTmpBuffer(true);
  recycler->takeRecycle(BufferRecycleType::HOST_CLEAR, ckrn_buf, sz, sz, COM_NUM_CHANNELS_COLOR);
  PixelsRect ckrn_rect = PixelsRect(ckrn_buf, kernelRect);
  PixelsRect *ckrn = &ckrn_rect;
  READ_DECL(ckrn);

  scale = 0.25f * sqrtf((float)(sz * sz));

  SET_COORDS(ckrn, 0, 0);
  while (ckrn_coords.y < sz) {
    v = 2.0f * (ckrn_coords.y / (float)sz) - 1.0f;
    while (ckrn_coords.x < sz) {
      u = 2.0f * (ckrn_coords.x / (float)sz) - 1.0f;
      r = (u * u + v * v) * scale;
      d = -sqrtf(sqrtf(sqrtf(r))) * 9.0f;
      fcol = CCL::make_float4_1(expf(d));
      // linear window good enough here, visual result counts, not scientific analysis
      // w = (1.0f-fabs(u))*(1.0f-fabs(v));
      // actually, Hanning window is ok, cos^2 for some reason is slower
      w = (0.5f + 0.5f * cosf(u * (float)M_PI)) * (0.5f + 0.5f * cosf(v * (float)M_PI));
      fcol *= w;
      WRITE_IMG4(ckrn, fcol);
      INCR1_COORDS_X(ckrn);
    }
    INCR1_COORDS_Y(ckrn);
    UPDATE_COORDS_X(ckrn, 0);
  }

  convolve(dst, src, ckrn_rect, man);

  recycler->giveRecycle(ckrn_buf);
}
