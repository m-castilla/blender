#ifndef __COM_KERNEL_COLOR_H__
#define __COM_KERNEL_COLOR_H__

/* All this methods has been taken from blendlib math_color.c file.
 * They are adapted for kernel compatibility with float4 vectors and normalized colors */
#include "kernel_util/COM_kernel_math.h"

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

CCL_NAMESPACE_BEGIN

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

ccl_device_inline float4 hsl_to_rgb(float4 hsl)
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
ccl_device_inline float4 rgb_to_yuv(float4 rgb)
{
  float4 yuv;
  yuv.x = 0.2126f * rgb.x + 0.7152f * rgb.y + 0.0722f * rgb.z;
  yuv.y = -0.09991f * rgb.x - 0.33609f * rgb.y + 0.436f * rgb.z;
  yuv.z = 0.615f * rgb.x - 0.55861f * rgb.y - 0.05639f * rgb.z;
  yuv.w = rgb.w;
  return yuv;
}

// In compositor only BLI_YUV_ITU_BT709 color space is used
ccl_device_inline float4 yuv_to_rgb(float4 yuv)
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

  // rgb = 255.0f * rgb;
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
  ycc.w = rgb.w;
  return ycc;
}

/* May have a bug. See FIXME comment in blendlib math_color.c */
ccl_device_inline float4 ycc_to_rgb(float4 ycc, int colorspace)
{
  float4 rgb;
  switch (colorspace) {
    case BLI_YCC_ITU_BT601:
      rgb.x = 1.164f * (ycc.x - 16.0f) + 1.596f * (ycc.y - 128.0f);
      rgb.y = 1.164f * (ycc.x - 16.0f) - 0.813f * (ycc.y - 128.0f) - 0.392f * (ycc.z - 128.0f);
      rgb.z = 1.164f * (ycc.x - 16.0f) + 2.017f * (ycc.z - 128.0f);
      break;
    case BLI_YCC_ITU_BT709:
      rgb.x = 1.164f * (ycc.x - 16.0f) + 1.793f * (ycc.y - 128.0f);
      rgb.y = 1.164f * (ycc.x - 16.0f) - 0.534f * (ycc.y - 128.0f) - 0.213f * (ycc.z - 128.0f);
      rgb.z = 1.164f * (ycc.x - 16.0f) + 2.115f * (ycc.z - 128.0f);
      break;
    case BLI_YCC_JFIF_0_255:
      rgb.x = ycc.x + 1.402f * ycc.y - 179.456f;
      rgb.y = ycc.x - 0.34414f * ycc.z - 0.71414f * ycc.y + 135.45984f;
      rgb.z = ycc.x + 1.772f * ycc.z - 226.816f;
      break;
    default:
      kernel_assert(!"invalid colorspace");
      break;
  }
  // rgb = rgb / 255.0f;
  rgb.w = ycc.w;
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

ccl_device_inline float4 rgb_to_hsl(float4 rgb)
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

ccl_device_inline float srgb_to_linearrgb(float c)
{
  if (c < 0.04045f) {
    return (c < 0.0f) ? 0.0f : c * (1.0f / 12.92f);
  }
  else {
    return powf((c + 0.055f) * (1.0f / 1.055f), 2.4f);
  }
}

ccl_device_inline float linearrgb_to_srgb(float c)
{
  if (c < 0.0031308f) {
    return (c < 0.0f) ? 0.0f : c * 12.92f;
  }
  else {
    return 1.055f * powf(c, 1.0f / 2.4f) - 0.055f;
  }
}

CCL_NAMESPACE_END

#endif
