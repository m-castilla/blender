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

#ifndef __COM_KERNEL_SAMPLING_H__
#define __COM_KERNEL_SAMPLING_H__

#ifndef __KERNEL_COMPUTE__

#  ifndef __COM_KERNEL_CPU_H__
#    error "Do not include this file directly, include COM_kernel_cpu.h instead."
#  endif

#  include "COM_Pixels.h"

#  include "kernel_util/COM_kernel_sampling_impl.h"

CCL_NAMESPACE_BEGIN

/* CLIP macros used for max performance (DO NOT DELETE, IT'S BEEN PROFILED THAT THEY IMPROVE
 * PERFORMANCE IN VERY DEEP LOOPS)*/
#  define READ_NEAREST1_CLIP(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_CLIP__(ctx_num, src, sampler, dst_pix, src##_coords, I, 1);

#  define READ_NEAREST3_CLIP(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_CLIP__(ctx_num, src, sampler, dst_pix, src##_coords, I, 3);

#  define READ_NEAREST4_CLIP(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_CLIP__(ctx_num, src, sampler, dst_pix, src##_coords, I, 4);

#  define SAMPLE_NEAREST1_CLIP(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_CLIP__(ctx_num, src, sampler, dst_pix, src##_coordsf, F, 1);

#  define SAMPLE_NEAREST3_CLIP(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_CLIP__(ctx_num, src, sampler, dst_pix, src##_coordsf, F, 3);

#  define SAMPLE_NEAREST4_CLIP(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_CLIP__(ctx_num, src, sampler, dst_pix, src##_coordsf, F, 4);

#  define SAMPLE_BILINEAR1_CLIP(ctx_num, src, sampler, dst_pix) \
    BILINEAR_CLIP__( \
        src##_bili_##ctx_num, src##_img, sampler, dst_pix, src##_coordsf.x, src##_coordsf.y, 1); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE1__(src##_bili_##ctx_num, src##_img, dst_pix); \
    EXTEND_CLIP_END; \
    EXTEND_CLIP_END;

#  define SAMPLE_BILINEAR3_CLIP(ctx_num, src, sampler, dst_pix) \
    BILINEAR_CLIP__( \
        src##_bili_##ctx_num, src##_img, sampler, dst_pix, src##_coordsf.x, src##_coordsf.y, 3); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE3__(src##_bili_##ctx_num, src##_img, dst_pix); \
    EXTEND_CLIP_END; \
    EXTEND_CLIP_END;

#  define SAMPLE_BILINEAR4_CLIP(ctx_num, src, sampler, dst_pix) \
    BILINEAR_CLIP__( \
        src##_bili_##ctx_num, src##_img, sampler, dst_pix, src##_coordsf.x, src##_coordsf.y, 4); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE4__(src##_bili_##ctx_num, src##_img, dst_pix); \
    EXTEND_CLIP_END; \
    EXTEND_CLIP_END;
/* END of CLIP macros */

ccl_device_inline bool check_extend(
    const PixelsImg &img, CCL::float4 &dst, const PixelsSampler &sampler, float &x, float &y)
{
  switch (sampler.extend) {
    case PixelExtend::UNCHECKED:
      if (x < img.start_xf || x >= img.end_xf || y < img.start_yf || y >= img.end_yf) {
        kernel_assert(!"Given coordinates are out of buffer range");
      }
      break;
    case PixelExtend::CLIP:
      /* clip result outside rect is zero */
      if (x < img.start_xf || x >= img.end_xf || y < img.start_yf || y >= img.end_yf) {
        dst.x = dst.y = dst.z = dst.w = 0.0f;
        return true;
      }
      break;
    case PixelExtend::EXTEND:
      if (x < img.start_xf) {
        x = img.start_xf;
      }
      else if (x >= img.end_xf) {
        x = img.end_xf - 1.0f;
      }
      if (y < img.start_yf) {
        y = img.start_yf;
      }
      else if (y >= img.end_yf) {
        y = img.end_yf - 1.0f;
      }
      break;
    case PixelExtend::REPEAT:
      if (x < img.start_xf || x >= img.end_xf) {
        x = fmodf(x - img.start_xf, img.row_elems) + img.start_xf;
        if (x < img.start_xf) {
          x += img.row_elems;
          if (x < img.start_xf) {
            x = img.start_xf;
          }
        }
      }

      if (y < img.start_yf || y >= img.end_yf) {
        y = fmodf(y - img.start_yf, img.col_elems) + img.start_yf;
        if (y < img.start_yf) {
          y += img.col_elems;
          if (y < img.start_yf) {
            y = img.start_yf;
          }
        }
      }
      break;
    case PixelExtend::MIRROR:
      if (x < img.start_xf || x >= img.end_xf) {
        int mirror_idx = (int)fabsf(x - img.start_xf) / img.row_elems;
        x = fmodf(x - img.start_xf, img.row_elems) + img.start_xf;
        if (x < img.start_xf) {
          x += img.row_elems;
          mirror_idx++;
        }
        if (mirror_idx % 2 == 1) {
          x = (img.row_elems - 1) - x;
        }
        if (x < img.start_xf) {
          x = img.start_xf;
        }
      }

      if (y < img.start_yf || y >= img.end_yf) {
        int mirror_idx = (int)fabsf(y - img.start_yf) / img.col_elems;
        y = fmodf(y - img.start_yf, img.col_elems) + img.start_yf;
        if (y < img.start_yf) {
          y += img.col_elems;
          mirror_idx++;
        }
        if (mirror_idx % 2 == 1) {
          y = (img.col_elems - 1) - y;
        }
        if (y < img.start_yf) {
          y = img.start_yf;
        }
      }
      break;
  }
  kernel_assert(x >= img.start_xf && x < img.end_xf && y >= img.start_yf && y < img.end_yf);

  return false;
}

ccl_device_inline void read_nearest(
    const PixelsImg &src_img, CCL::float4 &dst, const PixelsSampler &sampler, float u, float v)
{
  if (check_extend(src_img, dst, sampler, u, v)) {
    return;
  }

  size_t offset = (size_t)v * src_img.brow_chs_incr + (size_t)u * src_img.belem_chs_incr;
  if (src_img.elem_chs == 4) {
    dst = make_float4(src_img.buffer[offset],
                      src_img.buffer[offset + 1],
                      src_img.buffer[offset + 2],
                      src_img.buffer[offset + 3]);
  }
  else if (src_img.elem_chs == 1) {
    dst.x = src_img.buffer[offset];
  }
  else {
    dst = make_float4(
        src_img.buffer[offset], src_img.buffer[offset + 1], src_img.buffer[offset + 2], 0.0f);
  }
}

ccl_device_inline void read_bilinear(
    const PixelsImg &src_img, CCL::float4 &dst, const PixelsSampler &sampler, float u, float v)
{
  if (check_extend(src_img, dst, sampler, u, v)) {
    return;
  }

  float a, b;
  float a_b, ma_b, a_mb, ma_mb;

  float x1 = floorf(u);
  float x2 = ceilf(u);
  float y1 = floorf(v);
  float y2 = ceilf(v);

  a = u - x1;  // x1 == floorf(u)
  b = v - y1;  // y1 == floorf(v)
  a_b = a * b;
  ma_b = (1.0f - a) * b;
  a_mb = a * (1.0f - b);
  ma_mb = (1.0f - a) * (1.0f - b);

  /* check ceil boundaries extend. x1 and y1 were already checked at the start, floor would not
   * change the result of "check(dst, u, v)"*/
  if (check_extend(src_img, dst, sampler, x2, y2)) {
    return;
  }

  /* sample including outside of edges of image */
  size_t pix1_offset = (size_t)y1 * src_img.brow_chs_incr + (size_t)x1 * src_img.belem_chs_incr;
  size_t pix2_offset = (size_t)y2 * src_img.brow_chs_incr + (size_t)x1 * src_img.belem_chs_incr;
  size_t pix3_offset = (size_t)y1 * src_img.brow_chs_incr + (size_t)x2 * src_img.belem_chs_incr;
  size_t pix4_offset = (size_t)y2 * src_img.brow_chs_incr + (size_t)x2 * src_img.belem_chs_incr;
  if (src_img.elem_chs == 4) {
    float4 pix1 = make_float4(src_img.buffer[pix1_offset],
                              src_img.buffer[pix1_offset + 1],
                              src_img.buffer[pix1_offset + 2],
                              src_img.buffer[pix1_offset + 3]);

    float4 pix2 = make_float4(src_img.buffer[pix2_offset],
                              src_img.buffer[pix2_offset + 1],
                              src_img.buffer[pix2_offset + 2],
                              src_img.buffer[pix2_offset + 3]);

    float4 pix3 = make_float4(src_img.buffer[pix3_offset],
                              src_img.buffer[pix3_offset + 1],
                              src_img.buffer[pix3_offset + 2],
                              src_img.buffer[pix3_offset + 3]);

    float4 pix4 = make_float4(src_img.buffer[pix4_offset],
                              src_img.buffer[pix4_offset + 1],
                              src_img.buffer[pix4_offset + 2],
                              src_img.buffer[pix4_offset + 3]);

    dst = ma_mb * pix1 + a_mb * pix3 + ma_b * pix2 + a_b * pix4;
  }
  else if (src_img.elem_chs == 1) {
    dst.x = ma_mb * src_img.buffer[pix1_offset] + a_mb * src_img.buffer[pix3_offset] +
            ma_b * src_img.buffer[pix2_offset] + a_b * src_img.buffer[pix4_offset];
  }
  else {
    float3 pix1 = make_float3(src_img.buffer[pix1_offset],
                              src_img.buffer[pix1_offset + 1],
                              src_img.buffer[pix1_offset + 2]);

    float3 pix2 = make_float3(src_img.buffer[pix2_offset],
                              src_img.buffer[pix2_offset + 1],
                              src_img.buffer[pix2_offset + 2]);

    float3 pix3 = make_float3(src_img.buffer[pix3_offset],
                              src_img.buffer[pix3_offset + 1],
                              src_img.buffer[pix3_offset + 2]);

    float3 pix4 = make_float3(src_img.buffer[pix4_offset],
                              src_img.buffer[pix4_offset + 1],
                              src_img.buffer[pix4_offset + 2]);

    float3 rgb = ma_mb * pix1 + a_mb * pix3 + ma_b * pix2 + a_b * pix4;
    dst = make_float4_31(rgb, dst.w);
  }
}

ccl_device_inline void sample(const PixelsImg &img,
                              CCL::float4 &dst,
                              const PixelsSampler &sampler,
                              CCL::float2 coords)
{
  kernel_assert(sampler.filter == PixelInterpolation::BILINEAR ||
                sampler.filter == PixelInterpolation::NEAREST);
  if (sampler.filter == PixelInterpolation::BILINEAR) {
    read_bilinear(img, dst, sampler, coords.x, coords.y);
  }
  else {
    read_nearest(img, dst, sampler, coords.x, coords.y);
  }
}

/**
 * .May be used for checking the extend only
 *
 * \param img
 * \param dst
 * \param sampler
 * \param coords
 * \return
 */
ccl_device_inline void sample(const PixelsImg &img,
                              CCL::float4 &dst,
                              const PixelsSampler &sampler,
                              CCL::int2 coords)
{
  // If not NEAREST, float2 should be used
  kernel_assert(sampler.filter == PixelInterpolation::NEAREST);
  read_nearest(img, dst, sampler, coords.x, coords.y);
}

CCL_NAMESPACE_END

#else  //__KERNEL_COMPUTE__
#  define READ_NEAREST1_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_INT_IMG(src, sampler, dst_pix);
#  define READ_NEAREST3_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_INT_IMG(src, sampler, dst_pix);
#  define READ_NEAREST4_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_INT_IMG(src, sampler, dst_pix);
#  define SAMPLE_NEAREST1_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, dst_pix);
#  define SAMPLE_NEAREST3_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, dst_pix);
#  define SAMPLE_NEAREST4_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, dst_pix);
#  define SAMPLE_BILINEAR1_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, dst_pix);
#  define SAMPLE_BILINEAR3_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, dst_pix);
#  define SAMPLE_BILINEAR4_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, dst_pix);
#endif

#endif
