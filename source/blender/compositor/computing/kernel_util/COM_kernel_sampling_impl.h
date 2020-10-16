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

#ifndef __COM_KERNEL_SAMPLING_IMPL_H__
#define __COM_KERNEL_SAMPLING_IMPL_H__

#ifndef __COM_KERNEL_SAMPLING_H__
#  error "Do not include this file directly, include COM_kernel_sampling.h instead."
#endif

#ifndef __KERNEL_COMPUTE__

#  include "COM_Pixels.h"
#  include "kernel_util/COM_kernel_types.h"

/* SAMPLER PRIVATE IMPLEMENTATION MACROS */

#  define EXTEND_CLIP_I_START(img, dst, x__, y__) \
    if (x__ < img.start_x || x__ >= img.end_x || y__ < img.start_y || y__ >= img.end_y) { \
      dst.x = 0.0f; \
    } \
    else {

#  define EXTEND_CLIP_F_START(img, dst, x__, y) \
    if (x__ < img.start_xf || x__ >= img.end_xf || y < img.start_yf || y >= img.end_yf) { \
      dst.x = 0.0f; \
    } \
    else {

#  define EXTEND_CLIP_END }

#  define SAMPLER_ASSERT__(sampler, interp_mode, extend_mode) \
    kernel_assert(sampler.filter == PixelInterpolation::interp_mode && \
                  sampler.extend == PixelExtend::extend_mode); \
    (void)sampler;

#  define NEAREST_OFFSET__(name, coords, img) \
    size_t name##_offset__ = ((size_t)coords.y) * img.brow_chs_incr + \
                             ((size_t)coords.x) * img.belem_chs_incr;

#  define NEAREST_WRITE_F1__(name, img, coords, dst_pix) \
    NEAREST_OFFSET__(name, coords, img); \
    dst_pix.x = img.buffer[name##_offset__];

#  define NEAREST_WRITE_F3__(name, img, coords, dst_pix) \
    NEAREST_OFFSET__(name, coords, img); \
    dst_pix = CCL::make_float4(img.buffer[name##_offset__], \
                               img.buffer[name##_offset__ + 1], \
                               img.buffer[name##_offset__ + 2], \
                               0.0f);

#  define NEAREST_WRITE_F4__(name, img, coords, dst_pix) \
    NEAREST_OFFSET__(name, coords, img); \
    dst_pix = CCL::make_float4(img.buffer[name##_offset__], \
                               img.buffer[name##_offset__ + 1], \
                               img.buffer[name##_offset__ + 2], \
                               img.buffer[name##_offset__ + 3]);

#  define SAMPLE_NEAREST_CLIP__(ctx_num, src, sampler, dst_pix, coords, type_letter, n_chs) \
    SAMPLER_ASSERT__(sampler, NEAREST, CLIP); \
    EXTEND_CLIP_##type_letter##_START(src##_img, dst_pix, coords.x, coords.y); \
    NEAREST_WRITE_F##n_chs##__(src##_nearest_##ctx_num, src##_img, coords, dst_pix); \
    EXTEND_CLIP_END;

// u and v will never be negative because we are checking extend before, so we can use
// casting instead of floor and ceil
#  define BILINEAR_CLIP__(name, img, sampler, dst_pix, u__, v__, n_chs) \
    SAMPLER_ASSERT__(sampler, BILINEAR, CLIP); \
    EXTEND_CLIP_F_START(img, dst_pix, u__, v__); \
    int name##_x1__ = (int)u__; \
    int name##_x2__ = u__ > (float)name##_x1__ ? name##_x1__ + 1 : name##_x1__; \
    int name##_y1__ = (int)v__; \
    int name##_y2__ = v__ > (float)name##_y1__ ? name##_y1__ + 1 : name##_y1__; \
    EXTEND_CLIP_F_START(img, dst_pix, name##_x2__, name##_y2__);

#  define BILINEAR_OFFSET__(name, img, u__, v__) \
    float name##_a__ = u__ - name##_x1__; \
    float name##_b__ = v__ - name##_y1__; \
    float name##_a_b__ = name##_a__ * name##_b__; \
    float name##_ma_b__ = (1.0f - name##_a__) * name##_b__; \
    float name##_a_mb__ = name##_a__ * (1.0f - name##_b__); \
    float name##_ma_mb__ = (1.0f - name##_a__) * (1.0f - name##_b__); \
    size_t name##_pix1_offset__ = name##_y1__ * img.brow_chs_incr + \
                                  name##_x1__ * img.belem_chs_incr; \
    size_t name##_pix2_offset__ = name##_y2__ > name##_y1__ ? \
                                      name##_pix1_offset__ + img.brow_chs_incr : \
                                      name##_pix1_offset__; \
    size_t name##_pix3_offset__ = name##_x2__ > name##_x1__ ? \
                                      name##_pix1_offset__ + img.belem_chs_incr : \
                                      name##_pix1_offset__; \
    size_t name##_pix4_offset__ = name##_y2__ > name##_y1__ ? \
                                      name##_pix3_offset__ + img.brow_chs_incr : \
                                      name##_pix3_offset__;

#  define BILINEAR_WRITE4__(name, img, dst_pix) \
    CCL::float4 name##_pix1__ = CCL::make_float4(img.buffer[name##_pix1_offset__], \
                                                 img.buffer[name##_pix1_offset__ + 1], \
                                                 img.buffer[name##_pix1_offset__ + 2], \
                                                 img.buffer[name##_pix1_offset__ + 3]); \
    CCL::float4 name##_pix2__ = CCL::make_float4(img.buffer[name##_pix2_offset__], \
                                                 img.buffer[name##_pix2_offset__ + 1], \
                                                 img.buffer[name##_pix2_offset__ + 2], \
                                                 img.buffer[name##_pix2_offset__ + 3]); \
    CCL::float4 name##_pix3__ = CCL::make_float4(img.buffer[name##_pix3_offset__], \
                                                 img.buffer[name##_pix3_offset__ + 1], \
                                                 img.buffer[name##_pix3_offset__ + 2], \
                                                 img.buffer[name##_pix3_offset__ + 3]); \
    CCL::float4 name##_pix4__ = CCL::make_float4(img.buffer[name##_pix4_offset__], \
                                                 img.buffer[name##_pix4_offset__ + 1], \
                                                 img.buffer[name##_pix4_offset__ + 2], \
                                                 img.buffer[name##_pix4_offset__ + 3]); \
    dst_pix = name##_ma_mb__ * name##_pix1__ + name##_a_mb__ * name##_pix3__ + \
              name##_ma_b__ * name##_pix2__ + name##_a_b__ * name##_pix4__;

#  define BILINEAR_WRITE3__(name, img, dst_pix) \
    CCL::float3 name##_pix1__ = CCL::make_float3(img.buffer[name##_pix1_offset__], \
                                                 img.buffer[name##_pix1_offset__ + 1], \
                                                 img.buffer[name##_pix1_offset__ + 2]); \
    CCL::float3 name##_pix2__ = CCL::make_float3(img.buffer[name##_pix2_offset__], \
                                                 img.buffer[name##_pix2_offset__ + 1], \
                                                 img.buffer[name##_pix2_offset__ + 2]); \
    CCL::float3 name##_pix3__ = CCL::make_float3(img.buffer[name##_pix3_offset__], \
                                                 img.buffer[name##_pix3_offset__ + 1], \
                                                 img.buffer[name##_pix3_offset__ + 2]); \
    CCL::float3 name##_pix4__ = CCL::make_float3(img.buffer[name##_pix4_offset__], \
                                                 img.buffer[name##_pix4_offset__ + 1], \
                                                 img.buffer[name##_pix4_offset__ + 2]); \
    CCL::float3 name##_rgb__ = name##_ma_mb__ * name##_pix1__ + name##_a_mb__ * name##_pix3__ + \
                               name##_ma_b__ * name##_pix2__ + name##_a_b__ * name##_pix4__; \
    dst_pix = CCL::make_float4_31(name##_rgb__, dst_pix.w);

#  define BILINEAR_WRITE1__(name, img, dst_pix) \
    dst_pix.x = name##_ma_mb__ * img.buffer[name##_pix1_offset__] + \
                name##_a_mb__ * img.buffer[name##_pix3_offset__] + \
                name##_ma_b__ * img.buffer[name##_pix2_offset__] + \
                name##_a_b__ * img.buffer[name##_pix4_offset__];

/* END OF PRIVATE IMPLEMENTATION MACROS */

#endif

#endif
