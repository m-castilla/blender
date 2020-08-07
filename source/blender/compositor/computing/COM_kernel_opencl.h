#ifndef __COM_KERNEL_OPENCL_H__
#define __COM_KERNEL_OPENCL_H__

#define __KERNEL_COMPUTE__
#define __KERNEL_OPENCL__

/* no namespaces in opencl */
#define CCL_NAMESPACE_BEGIN
#define CCL_NAMESPACE_END

#ifdef __CL_NOINLINE__
#  define ccl_noinline __attribute__((noinline))
#else
#  define ccl_noinline
#endif

/* in opencl all functions are device functions, so leave this empty */
#define ccl_kernel __kernel void
#define ccl_device
#define ccl_device_inline ccl_device
#define ccl_device_forceinline ccl_device
#define ccl_device_noinline ccl_device ccl_noinline
#define ccl_device_noinline_cpu ccl_device
#define ccl_may_alias
#define ccl_static_constant static __constant
#define ccl_constant __constant
#define ccl_global __global
#define ccl_local __local
#define ccl_local_param __local
#define ccl_private __private
#define ccl_restrict restrict
#define ccl_ref
#define ccl_align(n) __attribute__((aligned(n)))
#define ccl_optional_struct_init

#if __OPENCL_VERSION__ >= 200
#  define ccl_loop_no_unroll __attribute__((opencl_unroll_hint(1)))
#else
#  define ccl_loop_no_unroll
#endif

#define ccl_addr_space

#define ATTR_FALLTHROUGH

/* no assert in opencl */
#define kernel_assert(cond)

/* make_type definitions with opencl style element initializers */
#ifdef make_float2
#  undef make_float2
#endif
#ifdef make_float3
#  undef make_float3
#endif
#ifdef make_float3_1
#  undef make_float3_1
#endif
#ifdef make_float3_4
#  undef make_float3_4
#endif
#ifdef make_float4
#  undef make_float4
#endif
#ifdef make_float4_1
#  undef make_float4_1
#endif
#ifdef make_float4_3
#  undef make_float4_3
#endif
#ifdef make_int2
#  undef make_int2
#endif
#ifdef make_int3
#  undef make_int3
#endif
#ifdef make_int3_1
#  undef make_int3_1
#endif
#ifdef make_int4
#  undef make_int4
#endif
#ifdef make_int4_1
#  undef make_int4_1
#endif

#define make_float2(x, y) ((float2)(x, y))
#define make_float3(x, y, z) ((float3)(x, y, z))
#define make_float3_1(f) ((float3)(f, f, f))
#define make_float3_4(f4) ((float3)(f4.x, f4.y, f4.z))
#define make_float4(x, y, z, w) ((float4)(x, y, z, w))
#define make_float4_1(f) ((float4)(f, f, f, f))
#define make_float4_3(f3) ((float4)(f3, 0.0f))
#define make_int2(x, y) ((int2)(x, y))
#define make_int3(x, y, z) ((int3)(x, y, z))
#define make_int3_1(i) ((int3)(i, i, i))
#define make_int4(x, y, z, w) ((int4)(x, y, z, w))
#define make_int4_1(i) ((int4)(i, i, i, i))

/* math functions */
#define __uint_as_float(x) as_float(x)
#define __float_as_uint(x) as_uint(x)
#define __int_as_float(x) as_float(x)
#define __float_as_int(x) as_int(x)
#define powf(x, y) pow(((float)(x)), ((float)(y)))
#define fabsf(x) fabs(((float)(x)))
#define copysignf(x, y) copysign(((float)(x)), ((float)(y)))
#define asinf(x) asin(((float)(x)))
#define acosf(x) acos(((float)(x)))
#define atanf(x) atan(((float)(x)))
#define floorf(x) floor(((float)(x)))
#define ceilf(x) ceil(((float)(x)))
#define hypotf(x, y) hypot(((float)(x)), ((float)(y)))
#define atan2f(x, y) atan2(((float)(x)), ((float)(y)))
#define fmaxf(x, y) fmax(((float)(x)), ((float)(y)))
#define fminf(x, y) fmin(((float)(x)), ((float)(y)))
#define fmodf(x, y) fmod((float)(x), (float)(y))
#define sinhf(x) sinh(((float)(x)))
#define coshf(x) cosh(((float)(x)))
#define tanhf(x) tanh(((float)(x)))
#define roundf(x) round(((float)(x)))

/* Use native functions with possibly lower precision for performance,
 * no issues found so far. */
#if 1
#  define sinf(x) native_sin(((float)(x)))
#  define cosf(x) native_cos(((float)(x)))
#  define tanf(x) native_tan(((float)(x)))
#  define expf(x) native_exp(((float)(x)))
#  define sqrtf(x) native_sqrt(((float)(x)))
#  define logf(x) native_log(((float)(x)))
#  define rcp(x) native_recip(x)
#else
#  define sinf(x) sin(((float)(x)))
#  define cosf(x) cos(((float)(x)))
#  define tanf(x) tan(((float)(x)))
#  define expf(x) exp(((float)(x)))
#  define sqrtf(x) sqrt(((float)(x)))
#  define logf(x) log(((float)(x)))
#  define rcp(x) recip(x)
#endif

/* define NULL */
#define NULL 0

/* enable extensions */
#ifdef __KERNEL_CL_KHR_FP16__
#  pragma OPENCL EXTENSION cl_khr_fp16 : enable
#endif

#include "kernel_util/COM_kernel_algo.h"
#include "kernel_util/COM_kernel_color.h"
#include "kernel_util/COM_kernel_defines.h"
#include "kernel_util/COM_kernel_geom.h"
#include "kernel_util/COM_kernel_math.h"
#include "kernel_util/COM_kernel_types.h"

/* Kernel function signature macros*/
#define CCL_READ(image) \
  __read_only image2d_t image, const BOOL image##_single, const float4 image##_single_elem
#define CCL_WRITE(image) \
  __write_only image2d_t image, const int image##_start_x, const int image##_start_y
#define CCL_SAMPLER(sampler) const sampler_t sampler
/* END of OpenCL kernel function signature macros*/

#define READ_DECL(src) float4 src##_pix;
#define SAMPLE_DECL(src) float4 src##_pix;
#define WRITE_DECL(dst) \
  int2 dst##_coords; \
  int write_offset_x = get_global_id(0), write_offset_y = get_global_id(1);

#define COORDS_TO_OFFSET(coords) \
  coords.x = dst##_start_x + write_offset_x; \
  coords.y = dst##_start_y + write_offset_y;

/*src_img must be a image2d_t , coords must be int2*/
#define READ_IMG(src, coords, result) \
  result = src##_single ? src##_single_elem : read_imagef(src, coords);

/*src_img must be a image2d_t, sampler must be sampler_t, coords must be float2*/
#define SAMPLE_IMG(src, coords, sampler, result) \
  result = src##_single ? src##_single_elem : read_imagef(src, sampler, coords);

/*dst_img must be a image2d_t , coords must be int2, pixel must be float4*/
#define WRITE_IMG(dst, coords, pixel) write_imagef(dst, coords, pixel);

/*CPU op loop*/

#define CPU_LOOP_START(dst)

#define CPU_LOOP_END

/*END of CPU op loop*/

#endif
