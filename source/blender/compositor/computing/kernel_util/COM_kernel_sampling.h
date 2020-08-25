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

#ifndef __KERNEL_COMPUTE__

#  ifndef __COM_KERNEL_CPU_H__
#    error "Do not include this file directly, include COM_kernel_cpu.h instead."
#  endif

#  include "kernel_util/COM_kernel_sampling_impl.h"

/* NEAREST UNCHECKED */
#  define SAMPLE_NEAREST1_UNCHECKED(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coordsf, float, F, 1, UNCHECKED);

#  define READ_NEAREST1_UNCHECKED(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coords, int, I, 1, UNCHECKED);

#  define SAMPLE_NEAREST3_UNCHECKED(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coordsf, float, F, 3, UNCHECKED);

#  define READ_NEAREST3_UNCHECKED(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coords, int, I, 3, UNCHECKED);

#  define SAMPLE_NEAREST4_UNCHECKED(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coordsf, float, F, 4, UNCHECKED);

#  define READ_NEAREST4_UNCHECKED(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coords, int, I, 4, UNCHECKED);
/* END of NEAREST UNCHECKED*/

/* NEAREST CLIP */
#  define SAMPLE_NEAREST1_CLIP(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_CLIP__(ctx_num, src, sampler, dst_pix, src##_coordsf, float, F, 1);

#  define SAMPLE_NEAREST3_CLIP(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_CLIP__(ctx_num, src, sampler, dst_pix, src##_coordsf, float, F, 3);

#  define SAMPLE_NEAREST4_CLIP(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_CLIP__(ctx_num, src, sampler, dst_pix, src##_coordsf, float, F, 4);

#  define READ_NEAREST1_CLIP(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_CLIP__(ctx_num, src, sampler, dst_pix, src##_coords, int, I, 1);

#  define READ_NEAREST3_CLIP(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_CLIP__(ctx_num, src, sampler, dst_pix, src##_coords, int, I, 3);

#  define READ_NEAREST4_CLIP(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_CLIP__(ctx_num, src, sampler, dst_pix, src##_coords, int, I, 4);
/* END of NEAREST CLIP*/

/* NEAREST EXTEND */
#  define SAMPLE_NEAREST1_EXTEND(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coordsf, float, F, 1, EXTEND);

#  define READ_NEAREST1_EXTEND(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coords, int, I, 1, EXTEND);

#  define SAMPLE_NEAREST3_EXTEND(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coordsf, float, F, 3, EXTEND);

#  define READ_NEAREST3_EXTEND(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coords, int, I, 3, EXTEND);

#  define SAMPLE_NEAREST4_EXTEND(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coordsf, float, F, 4, EXTEND);

#  define READ_NEAREST4_EXTEND(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coords, int, I, 4, EXTEND);
/* END of NEAREST EXTEND*/

/* NEAREST REPEAT */
#  define SAMPLE_NEAREST1_REPEAT(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coordsf, float, F, 1, REPEAT);

#  define READ_NEAREST1_REPEAT(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coords, int, I, 1, REPEAT);

#  define SAMPLE_NEAREST3_REPEAT(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coordsf, float, F, 3, REPEAT);

#  define READ_NEAREST3_REPEAT(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coords, int, I, 3, REPEAT);

#  define SAMPLE_NEAREST4_REPEAT(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coordsf, float, F, 4, REPEAT);

#  define READ_NEAREST4_REPEAT(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coords, int, I, 4, REPEAT);
/* END of NEAREST REPEAT*/

/* NEAREST MIRROR */
#  define SAMPLE_NEAREST1_MIRROR(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coordsf, float, F, 1, MIRROR);

#  define READ_NEAREST1_MIRROR(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coords, int, I, 1, MIRROR);

#  define SAMPLE_NEAREST3_MIRROR(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coordsf, float, F, 3, MIRROR);

#  define READ_NEAREST3_MIRROR(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coords, int, I, 3, MIRROR);

#  define SAMPLE_NEAREST4_MIRROR(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coordsf, float, F, 4, MIRROR);

#  define READ_NEAREST4_MIRROR(ctx_num, src, sampler, dst_pix) \
    SAMPLE_NEAREST_MODE__(ctx_num, src, sampler, dst_pix, src##_coords, int, I, 4, MIRROR);
/* END of NEAREST MIRROR*/

/*** END of NEAREST MACROS ***/

/*** BILINEAR MACROS ***/
#  define SAMPLE_BILINEAR1_UNCHECKED(ctx_num, src, sampler, dst_pix) \
    BILINEAR_MODE__( \
        src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y, UNCHECKED); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE1__(src##_bili_##ctx_num, src##_img, dst_pix);

#  define SAMPLE_BILINEAR3_UNCHECKED(ctx_num, src, sampler, dst_pix) \
    BILINEAR_MODE__( \
        src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y, UNCHECKED); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE3__(src##_bili_##ctx_num, src##_img, dst_pix);

#  define SAMPLE_BILINEAR4_UNCHECKED(ctx_num, src, sampler, dst_pix) \
    BILINEAR_MODE__( \
        src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y, UNCHECKED); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE4__(src##_bili_##ctx_num, src##_img, dst_pix);

#  define SAMPLE_BILINEAR1_CLIP(ctx_num, src, sampler, dst_pix) \
    BILINEAR_CLIP__( \
        src##_bili_##ctx_num, src##_img, dst_pix, src##_coordsf.x, src##_coordsf.y, 1); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE1__(src##_bili_##ctx_num, src##_img, dst_pix); \
    EXTEND_CLIP_END; \
    EXTEND_CLIP_END;

#  define SAMPLE_BILINEAR3_CLIP(ctx_num, src, sampler, dst_pix) \
    BILINEAR_CLIP__( \
        src##_bili_##ctx_num, src##_img, dst_pix, src##_coordsf.x, src##_coordsf.y, 3); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE3__(src##_bili_##ctx_num, src##_img, dst_pix); \
    EXTEND_CLIP_END; \
    EXTEND_CLIP_END;

#  define SAMPLE_BILINEAR4_CLIP(ctx_num, src, sampler, dst_pix) \
    BILINEAR_CLIP__( \
        src##_bili_##ctx_num, src##_img, dst_pix, src##_coordsf.x, src##_coordsf.y, 4); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE4__(src##_bili_##ctx_num, src##_img, dst_pix); \
    EXTEND_CLIP_END; \
    EXTEND_CLIP_END;

#  define SAMPLE_BILINEAR1_EXTEND(ctx_num, src, sampler, dst_pix) \
    BILINEAR_MODE__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y, EXTEND); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE1__(src##_bili_##ctx_num, src##_img, dst_pix);

#  define SAMPLE_BILINEAR3_EXTEND(ctx_num, src, sampler, dst_pix) \
    BILINEAR_MODE__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y, EXTEND); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE3__(src##_bili_##ctx_num, src##_img, dst_pix);

#  define SAMPLE_BILINEAR4_EXTEND(ctx_num, src, sampler, dst_pix) \
    BILINEAR_MODE__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y, EXTEND); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE4__(src##_bili_##ctx_num, src##_img, dst_pix);

#  define SAMPLE_BILINEAR1_REPEAT(ctx_num, src, sampler, dst_pix) \
    BILINEAR_MODE__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y, REPEAT); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE1__(src##_bili_##ctx_num, src##_img, dst_pix);

#  define SAMPLE_BILINEAR3_REPEAT(ctx_num, src, sampler, dst_pix) \
    BILINEAR_MODE__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y, REPEAT); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE3__(src##_bili_##ctx_num, src##_img, dst_pix);

#  define SAMPLE_BILINEAR4_REPEAT(ctx_num, src, sampler, dst_pix) \
    BILINEAR_MODE__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y, REPEAT); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE4__(src##_bili_##ctx_num, src##_img, dst_pix);

#  define SAMPLE_BILINEAR1_MIRROR(ctx_num, src, sampler, dst_pix) \
    BILINEAR_MODE__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y, MIRROR); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE1__(src##_bili_##ctx_num, src##_img, dst_pix);

#  define SAMPLE_BILINEAR3_MIRROR(ctx_num, src, sampler, dst_pix) \
    BILINEAR_MODE__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y, MIRROR); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE3__(src##_bili_##ctx_num, src##_img, dst_pix);

#  define SAMPLE_BILINEAR4_MIRROR(ctx_num, src, sampler, dst_pix) \
    BILINEAR_MODE__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y, MIRROR); \
    BILINEAR_OFFSET__(src##_bili_##ctx_num, src##_img, src##_coordsf.x, src##_coordsf.y); \
    BILINEAR_WRITE4__(src##_bili_##ctx_num, src##_img, dst_pix);
/*** END of BILINEAR MACROS ***/

#  define SAMPLE_IMG_N__(src, sampler, dst_pix, n_chs) \
    kernel_assert(sampler.filter == PixelInterpolation::BILINEAR || \
                  sampler.filter == PixelInterpolation::NEAREST); \
    if (sampler.filter == PixelInterpolation::BILINEAR) { \
      if (sampler.extend == PixelExtend::CLIP) { \
        SAMPLE_BILINEAR##n_chs##_CLIP(0, src, sampler, dst_pix); \
      } \
      else if (sampler.extend == PixelExtend::EXTEND) { \
        SAMPLE_BILINEAR##n_chs##_EXTEND(0, src, sampler, dst_pix); \
      } \
      else if (sampler.extend == PixelExtend::REPEAT) { \
        SAMPLE_BILINEAR##n_chs##_REPEAT(0, src, sampler, dst_pix); \
      } \
      else if (sampler.extend == PixelExtend::MIRROR) { \
        SAMPLE_BILINEAR##n_chs##_MIRROR(0, src, sampler, dst_pix); \
      } \
      else { \
        kernel_assert(sampler.extend == PixelExtend::UNCHECKED); \
        SAMPLE_BILINEAR##n_chs##_UNCHECKED(0, src, sampler, dst_pix); \
      } \
    } \
    else { \
      if (sampler.extend == PixelExtend::CLIP) { \
        SAMPLE_NEAREST##n_chs##_UNCHECKED(0, src, sampler, dst_pix); \
      } \
      else if (sampler.extend == PixelExtend::EXTEND) { \
        SAMPLE_NEAREST##n_chs##_EXTEND(0, src, sampler, dst_pix); \
      } \
      else if (sampler.extend == PixelExtend::REPEAT) { \
        SAMPLE_NEAREST##n_chs##_REPEAT(0, src, sampler, dst_pix); \
      } \
      else if (sampler.extend == PixelExtend::MIRROR) { \
        SAMPLE_NEAREST##n_chs##_MIRROR(0, src, sampler, dst_pix); \
      } \
      else { \
        kernel_assert(sampler.extend == PixelExtend::UNCHECKED); \
        SAMPLE_NEAREST##n_chs##_UNCHECKED(0, src, sampler, dst_pix); \
      } \
    }

#  define SAMPLE_IMG1(src, sampler, dst_pix) SAMPLE_IMG_N__(src, sampler, dst_pix, 1)
#  define SAMPLE_IMG3(src, sampler, dst_pix) SAMPLE_IMG_N__(src, sampler, dst_pix, 3)
#  define SAMPLE_IMG4(src, sampler, dst_pix) SAMPLE_IMG_N__(src, sampler, dst_pix, 4)

CCL_NAMESPACE_BEGIN
ccl_device_inline void sample(const PixelsImg &src_img,
                              CCL::float4 &dst_pix,
                              const PixelsSampler &sampler,
                              CCL::float2 src_coordsf)
{
  if (src_img.elem_chs == 4) {
    SAMPLE_IMG4(src, sampler, dst_pix);
  }
  else if (src_img.elem_chs == 1) {
    SAMPLE_IMG1(src, sampler, dst_pix);
  }
  else {
    kernel_assert(src_img.elem_chs == 3);
    SAMPLE_IMG3(src, sampler, dst_pix);
  }
}
CCL_NAMESPACE_END

#else  //__KERNEL_COMPUTE__
/* NEAREST UNCHECKED */
#  define SAMPLE_NEAREST1_UNCHECKED(ctx_num, src, sampler, dst_pix) \
    SAMPLE_IMG(src, sampler, result);
#  define READ_NEAREST1_UNCHECKED(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_NEAREST3_UNCHECKED(ctx_num, src, sampler, dst_pix) \
    SAMPLE_IMG(src, sampler, result);
#  define READ_NEAREST3_UNCHECKED(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_NEAREST4_UNCHECKED(ctx_num, src, sampler, dst_pix) \
    SAMPLE_IMG(src, sampler, result);
#  define READ_NEAREST4_UNCHECKED(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
/* END of NEAREST UNCHECKED*/

/* NEAREST CLIP */
#  define SAMPLE_NEAREST1_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_NEAREST3_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_NEAREST4_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define READ_NEAREST1_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define READ_NEAREST3_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define READ_NEAREST4_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
/* END of NEAREST CLIP*/

/* NEAREST EXTEND */
#  define SAMPLE_NEAREST1_EXTEND(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define READ_NEAREST1_EXTEND(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_NEAREST3_EXTEND(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define READ_NEAREST3_EXTEND(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_NEAREST4_EXTEND(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define READ_NEAREST4_EXTEND(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
/* END of NEAREST EXTEND*/

/* NEAREST REPEAT */
#  define SAMPLE_NEAREST1_REPEAT(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define READ_NEAREST1_REPEAT(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_NEAREST3_REPEAT(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define READ_NEAREST3_REPEAT(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_NEAREST4_REPEAT(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define READ_NEAREST4_REPEAT(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
/* END of NEAREST REPEAT*/

/* NEAREST MIRROR */
#  define SAMPLE_NEAREST1_MIRROR(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define READ_NEAREST1_MIRROR(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_NEAREST3_MIRROR(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define READ_NEAREST3_MIRROR(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_NEAREST4_MIRROR(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define READ_NEAREST4_MIRROR(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
/* END of NEAREST MIRROR*/

/*** END of NEAREST MACROS ***/

/*** BILINEAR MACROS ***/
#  define SAMPLE_BILINEAR1_UNCHECKED(ctx_num, src, sampler, dst_pix) \
    SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_BILINEAR3_UNCHECKED(ctx_num, src, sampler, dst_pix) \
    SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_BILINEAR4_UNCHECKED(ctx_num, src, sampler, dst_pix) \
    SAMPLE_IMG(src, sampler, result);

#  define SAMPLE_BILINEAR1_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_BILINEAR3_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_BILINEAR4_CLIP(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);

#  define SAMPLE_BILINEAR1_EXTEND(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_BILINEAR3_EXTEND(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_BILINEAR4_EXTEND(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);

#  define SAMPLE_BILINEAR1_REPEAT(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_BILINEAR3_REPEAT(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_BILINEAR4_REPEAT(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);

#  define SAMPLE_BILINEAR1_MIRROR(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_BILINEAR3_MIRROR(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_BILINEAR4_MIRROR(ctx_num, src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
/*** END of BILINEAR MACROS ***/

#  define SAMPLE_IMG1(src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_IMG3(src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);
#  define SAMPLE_IMG4(src, sampler, dst_pix) SAMPLE_IMG(src, sampler, result);

#endif

#endif
