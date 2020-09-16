#ifndef __COM_KERNEL_GEOM_H__
#define __COM_KERNEL_GEOM_H__

#include "kernel_util/COM_kernel_math.h"

/* All this methods has been taken from blendlib math_geom.c file.
 * They are adapted for vectors and kernel compatibility */
CCL_NAMESPACE_BEGIN

#define CCL_KEY_LINEAR 0
#define CCL_KEY_CARDINAL 1
#define CCL_KEY_BSPLINE 2
#define CCL_KEY_CATMULL_ROM 3

ccl_device_inline float2 closest_to_line_v2(const float2 p, const float2 l1, const float2 l2)
{
  float2 u = l2 - l1;
  float2 h = p - l1;
  float lambda = dot(u, h) / dot(u, u);
  return l1 + u * lambda;
}

ccl_device_inline void key_curve_position_weights(const float t, float data[4], const int type)
{
  float t2, t3, fc;

  if (type == CCL_KEY_LINEAR) {
    data[0] = 0.0f;
    data[1] = -t + 1.0f;
    data[2] = t;
    data[3] = 0.0f;
  }
  else if (type == CCL_KEY_CARDINAL) {
    t2 = t * t;
    t3 = t2 * t;
    fc = 0.71f;

    data[0] = -fc * t3 + 2.0f * fc * t2 - fc * t;
    data[1] = (2.0f - fc) * t3 + (fc - 3.0f) * t2 + 1.0f;
    data[2] = (fc - 2.0f) * t3 + (3.0f - 2.0f * fc) * t2 + fc * t;
    data[3] = fc * t3 - fc * t2;
  }
  else if (type == CCL_KEY_BSPLINE) {
    t2 = t * t;
    t3 = t2 * t;

    data[0] = -0.16666666f * t3 + 0.5f * t2 - 0.5f * t + 0.16666666f;
    data[1] = 0.5f * t3 - t2 + 0.66666666f;
    data[2] = -0.5f * t3 + 0.5f * t2 + 0.5f * t + 0.16666666f;
    data[3] = 0.16666666f * t3;
  }
  else if (type == CCL_KEY_CATMULL_ROM) {
    t2 = t * t;
    t3 = t2 * t;
    fc = 0.5f;

    data[0] = -fc * t3 + 2.0f * fc * t2 - fc * t;
    data[1] = (2.0f - fc) * t3 + (fc - 3.0f) * t2 + 1.0f;
    data[2] = (fc - 2.0f) * t3 + (3.0f - 2.0f * fc) * t2 + fc * t;
    data[3] = fc * t3 - fc * t2;
  }
}

CCL_NAMESPACE_END

#endif
