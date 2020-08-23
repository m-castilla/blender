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

#ifndef __COM_PIXELSSAMPLING_H__
#define __COM_PIXELSSAMPLING_H__

#ifndef __COM_KERNEL_CPU_H__
#  error "Do not include this file directly, include COM_kernel_cpu.h instead."
#endif

#ifndef __KERNEL_COMPUTE__

#  include "COM_Pixels.h"
#  include "kernel_util/COM_kernel_math.h"

CCL_NAMESPACE_BEGIN

ccl_device_inline bool check_extend(
    const PixelsImg &img, CCL::float4 &dst, const PixelsSampler &sampler, float &x, float &y)
{
  if (x < img.start_xf || x >= img.end_xf) {
    switch (sampler.extend) {
      case PixelExtend::UNCHECKED:
        kernel_assert(!"Given coordinates are out of buffer range");
        break;
      case PixelExtend::CLIP:
        /* clip result outside rect is zero */
        dst.x = dst.y = dst.z = dst.w = 0.0f;
        return true;
        break;
      case PixelExtend::EXTEND:
        x = x < img.start_xf ? 0.0f : img.end_xf - 1.0f;
        break;
      case PixelExtend::REPEAT:
        x = fmodf(x - img.start_xf, img.row_elems) + img.start_xf;
        break;
      case PixelExtend::MIRROR:
        x = (img.end_xf - 1.0f) - (fmodf(x - img.start_xf, img.row_elems) + img.start_xf);
        break;
    }
  }
  if (y < img.start_yf || y >= img.end_yf) {
    switch (sampler.extend) {
      case PixelExtend::UNCHECKED:
        kernel_assert(!"Given coordinates are out of buffer range");
        break;
      case PixelExtend::CLIP:
        /* clip result outside rect is zero */
        dst.x = dst.y = dst.z = dst.w = 0.0f;
        return true;
        break;
      case PixelExtend::EXTEND:
        y = y < img.start_yf ? 0.0f : img.end_yf - 1.0f;
        break;
      case PixelExtend::REPEAT:
        y = fmodf(y - img.start_yf, img.col_elems) + img.start_yf;
        break;
      case PixelExtend::MIRROR:
        y = (img.end_yf - 1.0f) - (fmodf(y - img.start_yf, img.col_elems) + img.start_yf);
        break;
    }
  }

  return false;
}

ccl_device_inline bool check_extend(
    const PixelsImg &img, CCL::float4 &dst, const PixelsSampler &sampler, int &x, int &y)
{
  if (x < img.start_x || x >= img.end_x) {
    switch (sampler.extend) {
      case PixelExtend::UNCHECKED:
        kernel_assert(!"Given coordinates are out of buffer range");
        break;
      case PixelExtend::CLIP:
        /* clip result outside rect is zero */
        dst.x = dst.y = dst.z = dst.w = 0.0f;
        return true;
        break;
      case PixelExtend::EXTEND:
        x = x < img.start_x ? 0 : img.end_x - 1;
        break;
      case PixelExtend::REPEAT:
        x = ((x - img.start_x) % img.row_elems) + img.start_x;
        break;
      case PixelExtend::MIRROR:
        x = (img.end_x - 1) - (((x - img.start_x) % img.row_elems) + img.start_x);
        break;
    }
  }
  if (y < img.start_y || y >= img.end_y) {
    switch (sampler.extend) {
      case PixelExtend::UNCHECKED:
        kernel_assert(!"Given coordinates are out of buffer range");
        break;
      case PixelExtend::CLIP:
        /* clip result outside rect is zero */
        dst.x = dst.y = dst.z = dst.w = 0.0f;
        return true;
        break;
      case PixelExtend::EXTEND:
        y = y < img.start_y ? 0 : img.end_y - 1;
        break;
      case PixelExtend::REPEAT:
        y = ((y - img.start_y) % img.col_elems) + img.start_y;
        break;
      case PixelExtend::MIRROR:
        y = (img.end_y - 1) - (((y - img.start_y) % img.col_elems) + img.start_y);
        break;
    }
  }

  return false;
}

ccl_device_inline void read_nearest(
    const PixelsImg &src_img, CCL::float4 &dst, const PixelsSampler &sampler, int x, int y)
{
  if (check_extend(src_img, dst, sampler, x, y)) {
    return;
  }

  size_t offset = (size_t)y * src_img.brow_chs_incr + (size_t)x * src_img.elem_chs_incr;
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

ccl_device_inline void read_nearest(
    const PixelsImg &src_img, CCL::float4 &dst, const PixelsSampler &sampler, float u, float v)
{
  if (check_extend(src_img, dst, sampler, u, v)) {
    return;
  }

  size_t offset = (size_t)v * src_img.brow_chs_incr + (size_t)u * src_img.elem_chs_incr;
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

  int x1 = floorf(u);
  int x2 = ceilf(u);
  int y1 = floorf(v);
  int y2 = ceilf(v);

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
  size_t pix1_offset = y1 * src_img.brow_chs_incr + x1 * src_img.elem_chs_incr;
  size_t pix2_offset = pix1_offset + ((size_t)y2 - y1) * src_img.brow_chs_incr;
  size_t pix3_offset = pix1_offset + ((size_t)x2 - x1) * src_img.elem_chs_incr;
  size_t pix4_offset = pix3_offset + ((size_t)y2 - y1) * src_img.brow_chs_incr;
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

#endif

CCL_NAMESPACE_END

#endif
