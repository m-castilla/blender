#ifndef __COM_KERNEL_COLOR_H__
#define __COM_KERNEL_COLOR_H__

/* All this methods has been taken from blendlib math_color.c file.
 * They are adapted for kernel compatibility with float4 vectors and normalized colors */
#include "kernel_util/COM_kernel_geom.h"
#include "kernel_util/COM_kernel_math.h"

CCL_NAMESPACE_BEGIN

/* YCbCr */
#ifndef BLI_YCC_ITU_BT601
#  define BLI_YCC_ITU_BT601 0
#endif
#ifndef BLI_YCC_ITU_BT709
#  define BLI_YCC_ITU_BT709 1
#endif
#ifndef BLI_YCC_JFIF_0_255
#  define BLI_YCC_JFIF_0_255 2
#endif
/* END of YCbCr */

/* **************** ColorBand ********************* */
/* colormode */
#define CCL_COLBAND_BLEND_RGB 0
#define CCL_COLBAND_BLEND_HSV 1
#define CCL_COLBAND_BLEND_HSL 2

/* interpolation */
#define CCL_COLBAND_INTERP_LINEAR 0
#define CCL_COLBAND_INTERP_EASE 1
#define CCL_COLBAND_INTERP_B_SPLINE 2
#define CCL_COLBAND_INTERP_CARDINAL 3
#define CCL_COLBAND_INTERP_CONSTANT 4

/* color interpolation */
#define CCL_COLBAND_HUE_NEAR 0
#define CCL_COLBAND_HUE_FAR 1
#define CCL_COLBAND_HUE_CW 2
#define CCL_COLBAND_HUE_CCW 3
/* **************** END of ColorBand ********************* */

/* Textures */
#define TEXMAP_CLIP_MIN 1
#define TEXMAP_CLIP_MAX 2
/* END of Textures */

ccl_constant float4 BLACK_PIXEL = make_float4(0, 0, 0, 1);
ccl_constant float4 TRANSPARENT_PIXEL = make_float4(0, 0, 0, 0);

ccl_device_inline float4 premul_to_straight(const float4 premul)
{
  if (premul.w == 0.0f || premul.w == 1.0f) {
    return premul;
  }
  else {
    float4 result = premul * (1.0f / premul.w);
    result.w = premul.w;
    return result;
  }
}

ccl_device_inline float4 straight_to_premul(const float4 straight)
{
  float4 result = straight * straight.w;
  result.w = straight.w;
  return result;
}

ccl_device_inline float4 hsv_to_rgb(const float4 hsv)
{
  float4 rgb;

  rgb.x = fabsf(hsv.x * 6.0f - 3.0f) - 1.0f;
  rgb.y = 2.0f - fabsf(hsv.x * 6.0f - 2.0f);
  rgb.z = 2.0f - fabsf(hsv.x * 6.0f - 4.0f);

  rgb = clamp(rgb, 0.0f, 1.0f);
  rgb = ((rgb - 1.0f) * hsv.y + 1.0f) * hsv.z;
  rgb.w = hsv.w;

  return rgb;
}

ccl_device_inline float4 hsl_to_rgb(const float4 hsl)
{
  float4 rgb;

  rgb.x = fabsf(hsl.x * 6.0f - 3.0f) - 1.0f;
  rgb.y = 2.0f - fabsf(hsl.x * 6.0f - 2.0f);
  rgb.z = 2.0f - fabsf(hsl.x * 6.0f - 4.0f);

  rgb = clamp(rgb, 0.0f, 1.0f);

  float chroma = (1.0f - fabsf(2.0f * hsl.z - 1.0f)) * hsl.y;
  rgb = (rgb - 0.5f) * chroma + hsl.z;
  rgb.w = hsl.w;

  return rgb;
}

// In compositor only BLI_YUV_ITU_BT709 color space is used
ccl_device_inline float4 rgb_to_yuv(const float4 rgb)
{
  float4 yuv;
  yuv.x = 0.2126f * rgb.x + 0.7152f * rgb.y + 0.0722f * rgb.z;
  yuv.y = -0.09991f * rgb.x - 0.33609f * rgb.y + 0.436f * rgb.z;
  yuv.z = 0.615f * rgb.x - 0.55861f * rgb.y - 0.05639f * rgb.z;
  yuv.w = rgb.w;
  return yuv;
}

// In compositor only BLI_YUV_ITU_BT709 color space is used
ccl_device_inline float4 yuv_to_rgb(const float4 yuv)
{
  float4 rgb;
  rgb.x = yuv.x + 1.28033f * yuv.z;
  rgb.y = yuv.x - 0.21482f * yuv.y - 0.38059f * yuv.z;
  rgb.z = yuv.x + 2.12798f * yuv.y;
  rgb.w = yuv.w;
  return rgb;
}

/* The RGB inputs are supposed gamma corrected and in the range 0 - 1.0f
 */
ccl_device_inline float4 rgb_to_ycc(float4 rgb, int colorspace)
{
  float4 ycc;

  float alpha = rgb.w;
  rgb = 255.0f * rgb;
  switch (colorspace) {
    case BLI_YCC_ITU_BT601:
      ycc.x = (0.257f * rgb.x) + (0.504f * rgb.y) + (0.098f * rgb.z) + 16.0f;
      ycc.y = (-0.148f * rgb.x) - (0.291f * rgb.y) + (0.439f * rgb.z) + 128.0f;
      ycc.z = (0.439f * rgb.x) - (0.368f * rgb.y) - (0.071f * rgb.z) + 128.0f;
      break;
    case BLI_YCC_ITU_BT709:
      ycc.x = (0.183f * rgb.x) + (0.614f * rgb.y) + (0.062f * rgb.z) + 16.0f;
      ycc.y = (-0.101f * rgb.x) - (0.338f * rgb.y) + (0.439f * rgb.z) + 128.0f;
      ycc.z = (0.439f * rgb.x) - (0.399f * rgb.y) - (0.040f * rgb.z) + 128.0f;
      break;
    case BLI_YCC_JFIF_0_255:
      ycc.x = (0.299f * rgb.x) + (0.587f * rgb.y) + (0.114f * rgb.z);
      ycc.y = (-0.16874f * rgb.x) - (0.33126f * rgb.y) + (0.5f * rgb.z) + 128.0f;
      ycc.z = (0.5f * rgb.x) - (0.41869f * rgb.y) - (0.08131f * rgb.z) + 128.0f;
      break;
    default:
      kernel_assert(!"invalid colorspace");
      break;
  }
  ycc /= 255.0f;
  ycc.w = alpha;
  return ycc;
}

/* May have a bug. See FIXME comment in blendlib math_color.c */
ccl_device_inline float4 ycc_to_rgb(float4 ycc, const int colorspace)
{
  float4 rgb;
  float alpha = ycc.w;
  ycc *= 255.0f;
  switch (colorspace) {
    case BLI_YCC_ITU_BT601:
      rgb.x = 1.164f * (ycc.x - 16.0f) + 1.596f * (ycc.z - 128.0f);
      rgb.y = 1.164f * (ycc.x - 16.0f) - 0.813f * (ycc.z - 128.0f) - 0.392f * (ycc.y - 128.0f);
      rgb.z = 1.164f * (ycc.x - 16.0f) + 2.017f * (ycc.y - 128.0f);
      break;
    case BLI_YCC_ITU_BT709:
      rgb.x = 1.164f * (ycc.x - 16.0f) + 1.793f * (ycc.z - 128.0f);
      rgb.y = 1.164f * (ycc.x - 16.0f) - 0.534f * (ycc.z - 128.0f) - 0.213f * (ycc.y - 128.0f);
      rgb.z = 1.164f * (ycc.x - 16.0f) + 2.115f * (ycc.y - 128.0f);
      break;
    case BLI_YCC_JFIF_0_255:
      rgb.x = ycc.x + 1.402f * ycc.z - 179.456f;
      rgb.y = ycc.x - 0.34414f * ycc.y - 0.71414f * ycc.z + 135.45984f;
      rgb.z = ycc.x + 1.772f * ycc.y - 226.816f;
      break;
    default:
      kernel_assert(!"invalid colorspace");
      break;
  }
  rgb = rgb / 255.0f;
  rgb.w = alpha;
  return rgb;
}

ccl_device_inline float4 rgb_to_hsv(float4 rgb)
{
  float k = 0.0f;
  float chroma;
  float min_gb;

  if (rgb.y < rgb.z) {
    SWAP(float, rgb.y, rgb.z);
    k = -1.0f;
  }
  min_gb = rgb.z;
  if (rgb.x < rgb.y) {
    SWAP(float, rgb.x, rgb.y);
    k = -2.0f / 6.0f - k;
    min_gb = fminf(rgb.y, rgb.z);
  }

  chroma = rgb.x - min_gb;

  float4 hsv;
  hsv.x = fabsf(k + (rgb.y - rgb.z) / (6.0f * chroma + 1e-20f));
  hsv.y = chroma / (rgb.x + 1e-20f);
  hsv.z = rgb.x;
  hsv.w = rgb.w;
  return hsv;
}

ccl_device_inline float4 rgb_to_hsl(const float4 rgb)
{
  float4 hsl;
  const float cmax = fmaxf3(rgb.x, rgb.y, rgb.z);
  const float cmin = fminf3(rgb.x, rgb.y, rgb.z);
  hsl.z = fminf(1.0, (cmax + cmin) / 2.0f);

  if (cmax == cmin) {
    hsl.x = hsl.y = 0.0f;  // achromatic
  }
  else {
    float d = cmax - cmin;
    hsl.y = hsl.z > 0.5f ? d / (2.0f - cmax - cmin) : d / (cmax + cmin);
    if (cmax == rgb.x) {
      hsl.x = (rgb.y - rgb.z) / d + (rgb.y < rgb.z ? 6.0f : 0.0f);
    }
    else if (cmax == rgb.y) {
      hsl.x = (rgb.z - rgb.x) / d + 2.0f;
    }
    else {
      hsl.x = (rgb.x - rgb.y) / d + 4.0f;
    }
  }
  hsl.x /= 6.0f;
  hsl.w = rgb.w;
  return hsl;
}

ccl_device_inline float srgb_to_linearrgb(const float c)
{
  if (c < 0.04045f) {
    return (c < 0.0f) ? 0.0f : c * (1.0f / 12.92f);
  }
  else {
    return powf((c + 0.055f) * (1.0f / 1.055f), 2.4f);
  }
}

ccl_device_inline float linearrgb_to_srgb(const float c)
{
  if (c < 0.0031308f) {
    return (c < 0.0f) ? 0.0f : c * 12.92f;
  }
  else {
    return 1.055f * powf(c, 1.0f / 2.4f) - 0.055f;
  }
}

ccl_device_inline int color_diff_threshold(const float4 a, const float4 b, const float threshold)
{
  return ((fabsf(a.x - b.x) > threshold) || (fabsf(a.y - b.y) > threshold) ||
          (fabsf(a.z - b.z) > threshold));
}

/**  COLORBAND **/
ccl_device_inline float colorband_hue_interp(
    const int ipotype_hue, const float mfac, const float fac, float h1, float h2)
{
  float h_interp;
  int mode = 0;

#define HUE_INTERP(h_a, h_b) ((mfac * (h_a)) + (fac * (h_b)))
#define HUE_MOD(h) (((h) < 1.0f) ? (h) : (h)-1.0f)

  h1 = HUE_MOD(h1);
  h2 = HUE_MOD(h2);

  kernel_assert(h1 >= 0.0f && h1 < 1.0f);
  kernel_assert(h2 >= 0.0f && h2 < 1.0f);

  switch (ipotype_hue) {
    case CCL_COLBAND_HUE_NEAR: {
      if ((h1 < h2) && (h2 - h1) > +0.5f) {
        mode = 1;
      }
      else if ((h1 > h2) && (h2 - h1) < -0.5f) {
        mode = 2;
      }
      else {
        mode = 0;
      }
      break;
    }
    case CCL_COLBAND_HUE_FAR: {
      /* Do full loop in Hue space in case both stops are the same... */
      if (h1 == h2) {
        mode = 1;
      }
      else if ((h1 < h2) && (h2 - h1) < +0.5f) {
        mode = 1;
      }
      else if ((h1 > h2) && (h2 - h1) > -0.5f) {
        mode = 2;
      }
      else {
        mode = 0;
      }
      break;
    }
    case CCL_COLBAND_HUE_CCW: {
      if (h1 > h2) {
        mode = 2;
      }
      else {
        mode = 0;
      }
      break;
    }
    case CCL_COLBAND_HUE_CW: {
      if (h1 < h2) {
        mode = 1;
      }
      else {
        mode = 0;
      }
      break;
    }
  }

  switch (mode) {
    case 0:
      h_interp = HUE_INTERP(h1, h2);
      break;
    case 1:
      h_interp = HUE_INTERP(h1 + 1.0f, h2);
      h_interp = HUE_MOD(h_interp);
      break;
    case 2:
      h_interp = HUE_INTERP(h1, h2 + 1.0f);
      h_interp = HUE_MOD(h_interp);
      break;
  }

  kernel_assert(h_interp >= 0.0f && h_interp < 1.0f);

#undef HUE_INTERP
#undef HUE_MOD

  return h_interp;
}

ccl_device_inline float4 colorband_evaluate(const float input_factor,
                                            const int n_bands,
                                            const int interp_type,
                                            const int hue_interp_type,
                                            const int color_mode,
                                            ccl_constant float4 *bands_colors,
                                            ccl_constant float *bands_pos)
{
  int cbd1, cbd2, cbd0, cbd3;
  int ipotype;

  if (n_bands == 0) {
    return make_float4(0.0f, 0.0f, 0.0f, 1.0f);
  }

  cbd1 = 0;

  /* Note: when ipotype >= COLBAND_INTERP_B_SPLINE,
   * we cannot do early-out with a constant color before first color stop and after last one,
   * because interpolation starts before and ends after those... */
  ipotype = (color_mode == CCL_COLBAND_BLEND_RGB) ? interp_type : CCL_COLBAND_INTERP_LINEAR;

  if (n_bands == 1) {
    return bands_colors[cbd1];
  }
  else if ((input_factor <= bands_pos[cbd1]) &&
           (ipotype == CCL_COLBAND_INTERP_LINEAR || ipotype == CCL_COLBAND_INTERP_EASE ||
            ipotype == CCL_COLBAND_INTERP_CONSTANT)) {
    /* We are before first color stop. */
    return bands_colors[cbd1];
  }
  else {
    // float4 left_color, right_color;
    // float left_pos, right_pos;
    int pos_idx;
    const float right_pos = 1.0f;
    const float left_pos = 0.0f;

    /* we're looking for first pos > in */
    for (pos_idx = 0; pos_idx < n_bands; pos_idx++) {
      if (bands_pos[pos_idx] > input_factor) {
        break;
      }
    }
    cbd1 += pos_idx;

    float pos1_value = bands_pos[cbd1];
    float pos2_value;
    if (pos_idx == n_bands) {
      cbd2 = cbd1 - 1;
      pos2_value = bands_pos[cbd2];
      cbd1 = cbd2;
      pos1_value = right_pos;
    }
    else if (pos_idx == 0) {
      cbd2 = cbd1;
      pos2_value = left_pos;
    }
    else {
      cbd2 = cbd1 - 1;
      pos2_value = bands_pos[cbd2];
    }

    if ((pos_idx == n_bands) &&
        (ipotype == CCL_COLBAND_INTERP_LINEAR || ipotype == CCL_COLBAND_INTERP_EASE ||
         ipotype == CCL_COLBAND_INTERP_CONSTANT)) {
      /* We are after last color stop. */
      return bands_colors[cbd2];
    }
    else if (ipotype == CCL_COLBAND_INTERP_CONSTANT) {
      /* constant */
      return bands_colors[cbd2];
    }
    else {
      float fac;
      if (pos2_value != pos1_value) {
        fac = (input_factor - pos1_value) / (pos2_value - pos1_value);
      }
      else {
        /* was setting to 0.0 in 2.56 & previous, but this
         * is incorrect for the last element, see [#26732] */
        fac = (pos_idx != n_bands) ? 0.0f : 1.0f;
      }

      if (ipotype == CCL_COLBAND_INTERP_B_SPLINE || ipotype == CCL_COLBAND_INTERP_CARDINAL) {
        /* ipo from right to left: 3 2 1 0 */
        float t[4];

        if (pos_idx >= n_bands - 1) {
          cbd0 = cbd1;
        }
        else {
          cbd0 = cbd1 + 1;
        }
        if (pos_idx < 2) {
          cbd3 = cbd2;
        }
        else {
          cbd3 = cbd2 - 1;
        }

        fac = clamp(fac, 0.0f, 1.0f);

        if (ipotype == CCL_COLBAND_INTERP_CARDINAL) {
          key_curve_position_weights(fac, t, KEY_CARDINAL);
        }
        else {
          key_curve_position_weights(fac, t, KEY_BSPLINE);
        }

        float4 result = t[3] * bands_colors[cbd3] + t[2] * bands_colors[cbd2] +
                        t[1] * bands_colors[cbd1] + t[0] * bands_colors[cbd0];
        result = clamp(result, 0.0f, 1.0f);
        return result;
      }
      else {
        if (ipotype == CCL_COLBAND_INTERP_EASE) {
          const float fac2 = fac * fac;
          fac = 3.0f * fac2 - 2.0f * fac2 * fac;
        }
        const float mfac = 1.0f - fac;

        if (UNLIKELY(color_mode == CCL_COLBAND_BLEND_HSV)) {
          float4 col1 = rgb_to_hsv(bands_colors[cbd1]);
          float4 col2 = rgb_to_hsv(bands_colors[cbd2]);
          float4 result;

          result.x = colorband_hue_interp(hue_interp_type, mfac, fac, col1.x, col2.x);
          result.y = mfac * col1.y + fac * col2.y;
          result.z = mfac * col1.z + fac * col2.z;
          result.w = mfac * bands_colors[cbd1].w + fac * bands_colors[cbd2].w;

          return hsv_to_rgb(result);
        }
        else if (UNLIKELY(color_mode == CCL_COLBAND_BLEND_HSL)) {
          float4 col1 = rgb_to_hsl(bands_colors[cbd1]);
          float4 col2 = rgb_to_hsl(bands_colors[cbd2]);
          float4 result;

          result.x = colorband_hue_interp(hue_interp_type, mfac, fac, col1.x, col2.x);
          result.y = mfac * col1.y + fac * col2.y;
          result.z = mfac * col1.z + fac * col2.z;
          result.w = mfac * bands_colors[cbd1].w + fac * bands_colors[cbd2].w;

          return hsl_to_rgb(result);
        }
        else {
          /* CCL_COLBAND_BLEND_RGB */
          return mfac * bands_colors[cbd1] + fac * bands_colors[cbd2];
        }
      }
    }
  }
}

/** END of COLORBAND **/

CCL_NAMESPACE_END

#endif
