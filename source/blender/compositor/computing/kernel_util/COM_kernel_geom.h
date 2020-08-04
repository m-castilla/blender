#ifndef __COM_KERNEL_GEOM_H__
#define __COM_KERNEL_GEOM_H__

#include "kernel_util/COM_kernel_math.h"

/* All this methods has been taken from blendlib math_geom.c file.
 * They are adapted for vectors and kernel compatibility */
CCL_NAMESPACE_BEGIN

ccl_device_inline float2 closest_to_line_v2(const float2 p, const float2 l1, const float2 l2)
{
  float2 u = l2 - l1;
  float2 h = p - l1;
  float lambda = dot(u, h) / dot(u, u);
  return l1 + u * lambda;
}

CCL_NAMESPACE_END

#endif
