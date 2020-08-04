#ifndef __COM_KERNEL_TYPES_H__
#define __COM_KERNEL_TYPES_H__

#ifndef __KERNEL_OPENCL__
#  include <stdlib.h>
#endif

#include "kernel_util/COM_kernel_defines.h"

/* Standard Integer Types */

#if !defined(__KERNEL_COMPUTE__) && !defined(_WIN32)
#  include <stdint.h>
#endif

#ifndef __KERNEL_COMPUTE__
#  include <cassert>

#  include "kernel_util/COM_kernel_optimization.h"
#  include "kernel_util/COM_kernel_simd.h"
#endif

CCL_NAMESPACE_BEGIN

/* Types
 *
 * Define simpler unsigned type names, and integer with defined number of bits.
 * Also vector types, named to be compatible with OpenCL builtin types, while
 * working for CUDA and C++ too. */

/* Shorter Unsigned Names */

#ifndef __KERNEL_OPENCL__
typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned short ushort;
#endif

/* There might be kernel types that don't support bool as kernel arguments (OpenCL for example),
 * we just use int in kernels which usually has better performance than other types*/
#define BOOL int
#define TRUE 1
#define FALSE 0

/* Fixed Bits Types */

#ifdef __KERNEL_OPENCL__
typedef ulong uint64_t;
#endif

#ifndef __KERNEL_COMPUTE__
#  ifdef _WIN32
typedef signed char int8_t;
typedef unsigned char uint8_t;

typedef signed short int16_t;
typedef unsigned short uint16_t;

typedef signed int int32_t;
typedef unsigned int uint32_t;

typedef long long int64_t;
typedef unsigned long long uint64_t;
#    ifdef __KERNEL_64_BIT__
typedef int64_t ssize_t;
#    else
typedef int32_t ssize_t;
#    endif
#  endif /* _WIN32 */

/* Generic Memory Pointer */

typedef uint64_t device_ptr;
#endif /* __KERNEL_COMPUTE__ */

ccl_device_inline size_t align_up(size_t offset, size_t alignment)
{
  return (offset + alignment - 1) & ~(alignment - 1);
}

ccl_device_inline size_t divide_up(size_t x, size_t y)
{
  return (x + y - 1) / y;
}

ccl_device_inline size_t round_up(size_t x, size_t multiple)
{
  return ((x + multiple - 1) / multiple) * multiple;
}

ccl_device_inline size_t round_down(size_t x, size_t multiple)
{
  return (x / multiple) * multiple;
}

ccl_device_inline bool is_power_of_two(size_t x)
{
  return (x & (x - 1)) == 0;
}

CCL_NAMESPACE_END

#ifndef __KERNEL_COMPUTE__

#  include "COM_kernel_types_int2.h"
#  include "COM_kernel_types_int3.h"
#  include "COM_kernel_types_int4.h"

#  include "COM_kernel_types_float2.h"
#  include "COM_kernel_types_float3.h"
#  include "COM_kernel_types_float4.h"

#  include "COM_kernel_types_int2_impl.h"
#  include "COM_kernel_types_int3_impl.h"
#  include "COM_kernel_types_int4_impl.h"

#  include "COM_kernel_types_float2_impl.h"
#  include "COM_kernel_types_float3_impl.h"
#  include "COM_kernel_types_float4_impl.h"

#endif

#endif
