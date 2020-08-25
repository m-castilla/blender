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

#ifndef __COM_PIXELSSAMPLING_IMPL_H__
#define __COM_PIXELSSAMPLING_IMPL_H__

#ifndef __COM_PIXELSSAMPLING_H__
#  error "Do not include this file directly, include COM_kernel_sampling.h instead."
#endif

#ifndef __KERNEL_COMPUTE__

#  include "COM_Pixels.h"
#  include "kernel_util/COM_kernel_types.h"

/* Extend macros */
#  define EXTEND_UNCHECKED_I(img, x__, y__) \
    kernel_assert(x__ >= img.start_x && x__ < img.end_x && y__ >= img.start_y && y__ < img.end_y);

#  define EXTEND_UNCHECKED_F(img, x__, y__) \
    kernel_assert(x__ >= img.start_xf && x__ < img.end_xf && y__ >= img.start_yf && \
                  y__ < img.end_yf);

#  define EXTEND_CLIP1_I_START(img, dst, x__, y__) \
    if (x__ < img.start_x || x__ >= img.end_x || y__ < img.start_y || y__ >= img.end_y) { \
      dst.x = 0.0f; \
    } \
    else {

#  define EXTEND_CLIP1_F_START(img, dst, x__, y) \
    if (x__ < img.start_xf || x__ >= img.end_xf || y < img.start_yf || y >= img.end_yf) { \
      dst.x = 0.0f; \
    } \
    else {

#  define EXTEND_CLIP3_I_START(img, dst, x__, y__) \
    if (x__ < img.start_x || x__ >= img.end_x || y__ < img.start_y || y__ >= img.end_y) { \
      dst.x = dst.y = dst.z = 0.0f; \
    } \
    else {

#  define EXTEND_CLIP3_F_START(img, dst, x__, y__) \
    if (x__ < img.start_xf || x__ >= img.end_xf || y__ < img.start_yf || y__ >= img.end_yf) { \
      dst.x = dst.y = dst.z = 0.0f; \
    } \
    else {

#  define EXTEND_CLIP4_I_START(img, dst, x__, y__) \
    if (x__ < img.start_x || x__ >= img.end_x || y__ < img.start_y || y__ >= img.end_y) { \
      dst.x = dst.y = dst.z = dst.w = 0.0f; \
    } \
    else {

#  define EXTEND_CLIP4_F_START(img, dst, x__, y__) \
    if (x__ < img.start_xf || x__ >= img.end_xf || y__ < img.start_yf || y__ >= img.end_yf) { \
      dst.x = dst.y = dst.z = dst.w = 0.0f; \
    } \
    else {

#  define EXTEND_CLIP_END }

#  define EXTEND_EXTEND_I(img, x__, y__) \
    x__ = x__ < img.start_x ? 0.0f : x__; \
    x__ = x__ >= img.end_x ? img.end_x - 1 : x__; \
    y__ = y__ < img.start_y ? 0.0f : y__; \
    y__ = y__ >= img.end_y ? img.end_y - 1 : y__;

#  define EXTEND_EXTEND_F(img, x__, y__) \
    x__ = x__ < img.start_xf ? 0.0f : x__; \
    x__ = x__ >= img.end_xf ? img.end_xf - 1.0f : x__; \
    y__ = y__ < img.start_yf ? 0.0f : y__; \
    y__ = y__ >= img.end_yf ? img.end_yf - 1.0f : y__;

#  define EXTEND_REPEAT_I(img, x__, y__) \
    x__ = (x__ < img.start_x || x__ >= img.end_x) ? \
              ((x__ - img.start_x) % img.row_elems) + img.start_x : \
              x__; \
    y__ = (y__ < img.start_y || y__ >= img.end_y) ? \
              ((y__ - img.start_y) % img.col_elems) + img.start_y : \
              y__;

#  define EXTEND_REPEAT_F(img, x__, y__) \
    x__ = (x__ < img.start_xf || x__ >= img.end_xf) ? \
              fmodf(x__ - img.start_xf, img.row_elems) + img.start_xf : \
              x__; \
    y__ = (y__ < img.start_yf || y__ >= img.end_yf) ? \
              fmodf(y__ - img.start_yf, img.col_elems) + img.start_yf : \
              y__;

#  define EXTEND_MIRROR_I(img, x__, y__) \
    x__ = (x__ < img.start_x || x__ >= img.end_x) ? \
              (img.end_x - 1) - (((x__ - img.start_x) % img.row_elems) + img.start_x) : \
              x__; \
    y__ = (y__ < img.start_y || y__ >= img.end_y) ? \
              (img.end_y - 1) - (((y__ - img.start_y) % img.col_elems) + img.start_y) : \
              y__;

#  define EXTEND_MIRROR_F(img, x__, y__) \
    x__ = (x__ < img.start_xf || x__ >= img.end_xf) ? \
              (img.end_xf - 1.0f) - (fmodf(x__ - img.start_xf, img.row_elems) + img.start_xf) : \
              x__; \
    y__ = (y__ < img.start_yf || y__ >= img.end_yf) ? \
              (img.end_yf - 1.0f) - (fmodf(y__ - img.start_yf, img.col_elems) + img.start_yf) : \
              y__;

/* END of Extend macros */

/* SAMPLER PRIVATE IMPLEMENTATION MACROS */

#  define SAMPLER_ASSERT__(sampler, interp_mode, extend_mode) \
    kernel_assert(sampler.interp == PixelInterpolation::interp_mode && \
                  sampler.extend == PixelExtend::extend_mode);

#  define NEAREST_OFFSET__(name, img) \
    size_t name##_offset__ = (size_t)name##_y__ * img.brow_chs_incr + \
                             (size_t)name##_x__ * img.elem_chs_incr;

#  define NEAREST_WRITE_F1__(name, img, dst_pix) \
    NEAREST_OFFSET__(name, img); \
    dst_pix.x = img.buffer[name##_offset__];

#  define NEAREST_WRITE_F3__(name, img, dst_pix) \
    NEAREST_OFFSET__(name, img); \
    dst_pix = CCL::make_float4(img.buffer[name##_offset__], \
                               img.buffer[name##_offset__ + 1], \
                               img.buffer[name##_offset__ + 2], \
                               0.0f);

#  define NEAREST_WRITE_F4__(name, img, dst_pix) \
    NEAREST_OFFSET__(name, img); \
    dst_pix = CCL::make_float4(img.buffer[name##_offset__], \
                               img.buffer[name##_offset__ + 1], \
                               img.buffer[name##_offset__ + 2], \
                               img.buffer[name##_offset__ + 3]);

#  define SAMPLE_NEAREST_CLIP__(ctx_num, src, sampler, dst_pix, coords, type, type_letter, n_chs) \
    SAMPLER_ASSERT__(sampler, NEAREST, CLIP); \
    type src##_nearest_##ctx_num##_x__ = coords.x; \
    type src##_nearest_##ctx_num##_y__ = coords.y; \
    EXTEND_CLIP##n_chs##_##type_letter##_START( \
        src##_img, dst_pix, src##_nearest_##ctx_num##_x__, src##_nearest_##ctx_num##_y__); \
    NEAREST_WRITE_##type_letter##n_chs##__(src##_nearest_##ctx_num, src##_img, dst_pix); \
    EXTEND_CLIP_END;

#  define SAMPLE_NEAREST_MODE__( \
      ctx_num, src, sampler, dst_pix, coords, type, type_letter, n_chs, mode) \
    SAMPLER_ASSERT__(sampler, NEAREST, mode); \
    type src##_nearest_##ctx_num##_x__ = coords.x; \
    type src##_nearest_##ctx_num##_y__ = coords.y; \
    EXTEND_##mode##_##type_letter##( \
        src##_img, src##_nearest_##ctx_num##_x__, src##_nearest_##ctx_num##_y__); \
    NEAREST_WRITE_##type_letter##n_chs##__(src##_nearest_##ctx_num, src##_img, dst_pix);

/* END OF PRIVATE IMPLEMENTATION MACROS */

// u and v will never be negative because we are checking extend before, so we can use
// casting instead of floor and ceil
#  define BILINEAR_CLIP__(name, img, dst_pix, u__, v__, n_chs) \
    SAMPLER_ASSERT__(sampler, BILINEAR, CLIP); \
    EXTEND_CLIP##n_chs##_F_START(img, dst_pix, u__, v__); \
    int name##_x1__ = (int)u__; \
    int name##_x2__ = u__ > (float)name##_x1__ ? name##_x1__ + 1 : name##_x1__; \
    int name##_y1__ = (int)v__; \
    int name##_y2__ = v__ > (float)name##_y1__ ? name##_y1__ + 1 : name##_y1__; \
    EXTEND_CLIP##n_chs##_F_START(img, dst_pix, name##_x2__, name##_y2__);

#  define BILINEAR_MODE__(name, img, u__, v__, extend_mode) \
    SAMPLER_ASSERT__(sampler, BILINEAR, extend_mode); \
    EXTEND_##extend_mode##_F(src##_img, u__, v__); \
    int name##_x1__ = (int)u__; \
    int name##_x2__ = u__ > (float)name##_x1__ ? name##_x1__ + 1 : name##_x1__; \
    int name##_y1__ = (int)v__; \
    int name##_y2__ = v__ > (float)name##_y1__ ? name##_y1__ + 1 : name##_y1__; \
    EXTEND_##extend_mode##_F(src##_img, name##_x2__, name##_y2__);

#  define BILINEAR_OFFSET__(name, img, u__, v__) \
    float name##_a__ = u__ - name##_x1__; \
    float name##_b__ = v__ - name##_y1__; \
    float name##_a_b__ = name##_a__ * name##_b__; \
    float name##_ma_b__ = (1.0f - name##_a__) * name##_b__; \
    float name##_a_mb__ = name##_a__ * (1.0f - name##_b__); \
    float name##_ma_mb__ = (1.0f - name##_a__) * (1.0f - name##_b__); \
    size_t name##_pix1_offset__ = name##_y1__ * img.brow_chs_incr + \
                                  name##_x1__ * img.elem_chs_incr; \
    size_t name##_pix2_offset__ = name##_y2__ > name##_y1__ ? \
                                      name##_pix1_offset__ + img.brow_chs_incr : \
                                      name##_pix1_offset__; \
    size_t name##_pix3_offset__ = name##_x2__ > name##_x1__ ? \
                                      name##_pix1_offset__ + img.elem_chs_incr : \
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

#endif

#endif
