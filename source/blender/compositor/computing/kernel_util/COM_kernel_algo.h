#ifndef __COM_KERNEL_ALGO_H__
#define __COM_KERNEL_ALGO_H__

#include "kernel_util/COM_kernel_color.h"
#include "kernel_util/COM_kernel_geom.h"
#include "kernel_util/COM_kernel_math.h"

CCL_NAMESPACE_BEGIN

#define CCL_MASK_TYPE_ADD 0
#define CCL_MASK_TYPE_SUBTRACT 1
#define CCL_MASK_TYPE_MULTIPLY 2
#define CCL_MASK_TYPE_NOT 3

ccl_device_inline float finv_test(const float f, const bool test)
{
  return (LIKELY(test == false)) ? f : 1.0f - f;
}

ccl_device_inline int max_channel_f3(const float3 f3)
{
  return ((f3.x > f3.y) ? ((f3.x > f3.z) ? 0 : 2) : ((f3.y > f3.z) ? 1 : 2));
}

ccl_device_inline int max_channel_f4_3(const float4 f4)
{
  return ((f4.x > f4.y) ? ((f4.x > f4.z) ? 0 : 2) : ((f4.y > f4.z) ? 1 : 2));
}

CCL_NAMESPACE_END

#endif
