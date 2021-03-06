#ifndef __COM_KERNEL_MATH_H__
#define __COM_KERNEL_MATH_H__

/* Math
 *
 * Basic math functions on scalar and vector types. This header is used by
 * both the kernel code when compiled as C++, and other C++ non-kernel code. */

#ifndef __KERNEL_COMPUTE__
#  include <cmath>
#endif

#ifndef __KERNEL_OPENCL__
#  include <float.h>
#  include <math.h>
#  include <stdio.h>
#endif /* __KERNEL_OPENCL__ */

#include "kernel_util/COM_kernel_types.h"

CCL_NAMESPACE_BEGIN

ccl_constant float4 ZERO_F4 = make_float4_1(0.0f);
ccl_constant float4 ONE_F4 = make_float4_1(1.0f);
ccl_constant float4 TRANSPARENT_PIXEL = make_float4_1(0.0f);
ccl_constant float4 BLACK_PIXEL = make_float4(0.0f, 0.0f, 0.0f, 1.0f);
ccl_constant float4 WHITE_PIXEL = make_float4(1.0f, 1.0f, 1.0f, 1.0f);
ccl_constant float2 ZERO_F2 = make_float2(0.0f, 0.0f);
ccl_constant float3 ZERO_F3 = make_float3_1(0.0f);
ccl_constant float2 ONE_F2 = make_float2(1.0f, 1.0f);
ccl_constant float3 ONE_F3 = make_float3_1(1.0f);

/* Float Pi variations */

/* Division */
#undef M_PI_F
#define M_PI_F (3.1415926535897932f) /* pi */

#undef M_PI_2_F
#define M_PI_2_F (1.5707963267948966f) /* pi/2 */

#undef M_PI_4_F
#define M_PI_4_F (0.7853981633974830f) /* pi/4 */

#undef M_1_PI_F
#define M_1_PI_F (0.3183098861837067f) /* 1/pi */

#undef M_2_PI_F
#define M_2_PI_F (0.6366197723675813f) /* 2/pi */

#undef M_1_2PI_F
#define M_1_2PI_F (0.1591549430918953f) /* 1/(2*pi) */

#undef M_SQRT_PI_8_F
#define M_SQRT_PI_8_F (0.6266570686577501f) /* sqrt(pi/8) */

#undef M_LN_2PI_F
#define M_LN_2PI_F (1.8378770664093454f) /* ln(2*pi) */

#undef M_SQRT1_2_F
#define M_SQRT1_2_F (0.707106781186547524401f) /* 1/sqrt(2)  */

/* Multiplication */
#undef M_2PI_F
#define M_2PI_F (6.2831853071795864f) /* 2*pi */

#undef M_4PI_F
#define M_4PI_F (12.566370614359172f) /* 4*pi */

/* Float sqrt variations */
#undef M_SQRT2_F
#define M_SQRT2_F (1.4142135623730950f) /* sqrt(2) */

#undef M_LN2_F
#define M_LN2_F (0.6931471805599453f) /* ln(2) */

#undef M_LN10_F
#define M_LN10_F (2.3025850929940457f) /* ln(10) */

#undef RAD2DEGF
#define RAD2DEGF(_rad) ((_rad) * (float)(180.0f / M_PI_F))

#undef DEG2RADF
#define DEG2RADF(_deg) ((_deg) * (float)(M_PI_F / 180.0f))

/* Scalar */

#ifndef __KERNEL_OPENCL__
ccl_device_inline float fmaxf(const float a, const float b)
{
  return (a > b) ? a : b;
}

ccl_device_inline float fminf(const float a, const float b)
{
  return (a < b) ? a : b;
}
#endif /* !__KERNEL_OPENCL__ */

#ifndef __KERNEL_COMPUTE__
using std::isfinite;
using std::isnan;
using std::sqrt;

ccl_device_inline int abs(const int x)
{
  return (x > 0) ? x : -x;
}

ccl_device_inline int max(const int a, const int b)
{
  return (a > b) ? a : b;
}

ccl_device_inline int min(const int a, const int b)
{
  return (a < b) ? a : b;
}

ccl_device_inline float max(const float a, const float b)
{
  return (a > b) ? a : b;
}

ccl_device_inline float min(const float a, const float b)
{
  return (a < b) ? a : b;
}
#endif /* __KERNEL_COMPUTE__ */

ccl_device_inline float fminf3(const float a, const float b, const float c)
{
  return fminf(fminf(a, b), c);
}

ccl_device_inline float fmaxf3(const float a, const float b, const float c)
{
  return fmaxf(fmaxf(a, b), c);
}

ccl_device_inline float fminf4(const float a, const float b, const float c, const float d)
{
  return fminf(fminf(a, b), fminf(c, d));
}

ccl_device_inline float fmaxf4(const float a, const float b, const float c, const float d)
{
  return fmaxf(fmaxf(a, b), fmaxf(c, d));
}

#ifndef __KERNEL_OPENCL__
/* Int/Float conversion */

ccl_device_inline int as_int(uint i)
{
  union {
    uint ui;
    int i;
  } u;
  u.ui = i;
  return u.i;
}

ccl_device_inline uint as_uint(int i)
{
  union {
    uint ui;
    int i;
  } u;
  u.i = i;
  return u.ui;
}

ccl_device_inline uint as_uint(float f)
{
  union {
    uint i;
    float f;
  } u;
  u.f = f;
  return u.i;
}

ccl_device_inline int __float_as_int(float f)
{
  union {
    int i;
    float f;
  } u;
  u.f = f;
  return u.i;
}

ccl_device_inline float __int_as_float(int i)
{
  union {
    int i;
    float f;
  } u;
  u.i = i;
  return u.f;
}

ccl_device_inline uint __float_as_uint(float f)
{
  union {
    uint i;
    float f;
  } u;
  u.f = f;
  return u.i;
}

ccl_device_inline float __uint_as_float(uint i)
{
  union {
    uint i;
    float f;
  } u;
  u.i = i;
  return u.f;
}

ccl_device_inline int4 __float4_as_int4(float4 f)
{
#  ifdef __KERNEL_SSE__
  return int4(_mm_castps_si128(f.m128));
#  else
  return make_int4(
      __float_as_int(f.x), __float_as_int(f.y), __float_as_int(f.z), __float_as_int(f.w));
#  endif
}

ccl_device_inline float4 __int4_as_float4(int4 i)
{
#  ifdef __KERNEL_SSE__
  return float4(_mm_castsi128_ps(i.m128));
#  else
  return make_float4(
      __int_as_float(i.x), __int_as_float(i.y), __int_as_float(i.z), __int_as_float(i.w));
#  endif
}
#endif /* __KERNEL_OPENCL__ */

/* Versions of functions which are safe for fast math. */
ccl_device_inline bool isnan_safe(const float f)
{
  unsigned int x = __float_as_uint(f);
  return (x << 1) > 0xff000000u;
}

ccl_device_inline bool isfinite_safe(const float f)
{
  /* By IEEE 754 rule, 2*Inf equals Inf */
  unsigned int x = __float_as_uint(f);
  return (f == f) && (x == 0 || x == (1u << 31) || (f != 2.0f * f)) && !((x << 1) > 0xff000000u);
}

ccl_device_inline float ensure_finite(const float v)
{
  return isfinite_safe(v) ? v : 0.0f;
}

#ifndef __KERNEL_OPENCL__
ccl_device_inline int clamp(const int a, const int mn, const int mx)
{
  return min(max(a, mn), mx);
}

ccl_device_inline float clamp(const float a, const float mn, const float mx)
{
  return fminf(fmaxf(a, mn), mx);
}

ccl_device_inline float mix(const float a, const float b, const float t)
{
  return a + t * (b - a);
}

ccl_device_inline float smoothstep(const float edge0, const float edge1, const float x)
{
  float result;
  if (x < edge0)
    result = 0.0f;
  else if (x >= edge1)
    result = 1.0f;
  else {
    float t = (x - edge0) / (edge1 - edge0);
    result = (3.0f - 2.0f * t) * (t * t);
  }
  return result;
}

#endif /* __KERNEL_OPENCL__ */

#ifndef __KERNEL_CUDA__
ccl_device_inline float saturate(const float a)
{
  return clamp(a, 0.0f, 1.0f);
}
#endif /* __KERNEL_CUDA__ */

ccl_device_inline int float_to_int(const float f)
{
  return (int)f;
}

ccl_device_inline int floor_to_int(const float f)
{
  return float_to_int(floorf(f));
}

ccl_device_inline int quick_floor_to_int(const float x)
{
  return float_to_int(x) - ((x < 0) ? 1 : 0);
}

ccl_device_inline float floorfrac(const float x, int *i)
{
  *i = quick_floor_to_int(x);
  return x - *i;
}

ccl_device_inline int ceil_to_int(const float f)
{
  return float_to_int(ceilf(f));
}

ccl_device_inline float fractf(const float x)
{
  return x - floorf(x);
}

/* Adapted from godotengine math_funcs.h. */
ccl_device_inline float wrapf(const float value, const float max, const float min)
{
  float range = max - min;
  return (range != 0.0f) ? value - (range * floorf((value - min) / range)) : min;
}

ccl_device_inline float pingpongf(const float a, const float b)
{
  return (b != 0.0f) ? fabsf(fractf((a - b) / (b * 2.0f)) * b * 2.0f - b) : 0.0f;
}

ccl_device_inline float smoothminf(const float a, const float b, const float k)
{
  if (k != 0.0f) {
    float h = fmaxf(k - fabsf(a - b), 0.0f) / k;
    return fminf(a, b) - h * h * h * k * (1.0f / 6.0f);
  }
  else {
    return fminf(a, b);
  }
}

ccl_device_inline float signf(const float f)
{
  return (f < 0.0f) ? -1.0f : 1.0f;
}

ccl_device_inline float nonzerof(const float f, const float eps)
{
  if (fabsf(f) < eps)
    return signf(f) * eps;
  else
    return f;
}

/* Signum function testing for zero. Matches GLSL and OSL functions. */
ccl_device_inline float compatible_signf(const float f)
{
  if (f == 0.0f) {
    return 0.0f;
  }
  else {
    return signf(f);
  }
}

ccl_device_inline float smoothstepf(const float f)
{
  float ff = f * f;
  return (3.0f * ff - 2.0f * ff * f);
}

ccl_device_inline int mod(const int x, const int m)
{
  return (x % m + m) % m;
}

ccl_device_inline float3 float2_to_float3(const float2 a)
{
  return make_float3(a.x, a.y, 0.0f);
}

ccl_device_inline float3 float4_to_float3(const float4 a)
{
  return make_float3(a.x, a.y, a.z);
}

ccl_device_inline float4 float3_to_float4(const float3 a)
{
  return make_float4(a.x, a.y, a.z, 1.0f);
}

ccl_device_inline float inverse_lerp(const float a, const float b, const float x)
{
  return (x - a) / (b - a);
}

/* Cubic interpolation between b and c, a and d are the previous and next point. */
ccl_device_inline float cubic_interp(
    const float a, const float b, const float c, const float d, const float x)
{
  return 0.5f *
             (((d + 3.0f * (b - c) - a) * x + (2.0f * a - 5.0f * b + 4.0f * c - d)) * x +
              (c - a)) *
             x +
         b;
}

CCL_NAMESPACE_END

#include "kernel_util/COM_kernel_types.h"

#include "kernel_util/COM_kernel_math_int2.h"
#include "kernel_util/COM_kernel_math_int3.h"
#include "kernel_util/COM_kernel_math_int4.h"

#include "kernel_util/COM_kernel_math_float2.h"
#include "kernel_util/COM_kernel_math_float3.h"
#include "kernel_util/COM_kernel_math_float4.h"

CCL_NAMESPACE_BEGIN

#ifndef __KERNEL_OPENCL__
/* Interpolation */

template<class A, class B> A lerp(const A &a, const A &b, const B &t)
{
  return (A)(a * ((B)1 - t) + b * t);
}

#endif /* __KERNEL_OPENCL__ */

/* Triangle */
#ifndef __KERNEL_OPENCL__
ccl_device_inline float triangle_area(const float3 &v1, const float3 &v2, const float3 &v3)
#else
ccl_device_inline float triangle_area(const float3 v1, const float3 v2, const float3 v3)
#endif
{
  return length(cross(v3 - v2, v1 - v2)) * 0.5f;
}

/* Orthonormal vectors */

ccl_device_inline void make_orthonormals(const float3 N, float3 *a, float3 *b)
{
#if 0
  if (fabsf(N.y) >= 0.999f) {
    *a = make_float3(1, 0, 0);
    *b = make_float3(0, 0, 1);
    return;
  }
  if (fabsf(N.z) >= 0.999f) {
    *a = make_float3(1, 0, 0);
    *b = make_float3(0, 1, 0);
    return;
  }
#endif

  if (N.x != N.y || N.x != N.z)
    *a = make_float3(N.z - N.y, N.x - N.z, N.y - N.x);  //(1,1,1)x N
  else
    *a = make_float3(N.z - N.y, N.x + N.z, -N.y - N.x);  //(-1,1,1)x N

  *a = normalize(*a);
  *b = cross(N, *a);
}

/* Color division */

ccl_device_inline float3 safe_invert_color(const float3 a)
{
  float x, y, z;

  x = (a.x != 0.0f) ? 1.0f / a.x : 0.0f;
  y = (a.y != 0.0f) ? 1.0f / a.y : 0.0f;
  z = (a.z != 0.0f) ? 1.0f / a.z : 0.0f;

  return make_float3(x, y, z);
}

ccl_device_inline float3 safe_divide_color(const float3 a, const float3 b)
{
  float x, y, z;

  x = (b.x != 0.0f) ? a.x / b.x : 0.0f;
  y = (b.y != 0.0f) ? a.y / b.y : 0.0f;
  z = (b.z != 0.0f) ? a.z / b.z : 0.0f;

  return make_float3(x, y, z);
}

ccl_device_inline float3 safe_divide_even_color(const float3 a, const float3 b)
{
  float x, y, z;

  x = (b.x != 0.0f) ? a.x / b.x : 0.0f;
  y = (b.y != 0.0f) ? a.y / b.y : 0.0f;
  z = (b.z != 0.0f) ? a.z / b.z : 0.0f;

  /* try to get gray even if b is zero */
  if (b.x == 0.0f) {
    if (b.y == 0.0f) {
      x = z;
      y = z;
    }
    else if (b.z == 0.0f) {
      x = y;
      z = y;
    }
    else
      x = 0.5f * (y + z);
  }
  else if (b.y == 0.0f) {
    if (b.z == 0.0f) {
      y = x;
      z = x;
    }
    else
      y = 0.5f * (x + z);
  }
  else if (b.z == 0.0f) {
    z = 0.5f * (x + y);
  }

  return make_float3(x, y, z);
}

/* Rotation of point around axis and angle */

ccl_device_inline float3 rotate_around_axis(const float3 p, const float3 axis, const float angle)
{
  float costheta = cosf(angle);
  float sintheta = sinf(angle);
  float3 r;

  r.x = ((costheta + (1 - costheta) * axis.x * axis.x) * p.x) +
        (((1 - costheta) * axis.x * axis.y - axis.z * sintheta) * p.y) +
        (((1 - costheta) * axis.x * axis.z + axis.y * sintheta) * p.z);

  r.y = (((1 - costheta) * axis.x * axis.y + axis.z * sintheta) * p.x) +
        ((costheta + (1 - costheta) * axis.y * axis.y) * p.y) +
        (((1 - costheta) * axis.y * axis.z - axis.x * sintheta) * p.z);

  r.z = (((1 - costheta) * axis.x * axis.z - axis.y * sintheta) * p.x) +
        (((1 - costheta) * axis.y * axis.z + axis.x * sintheta) * p.y) +
        ((costheta + (1 - costheta) * axis.z * axis.z) * p.z);

  return r;
}

/* NaN-safe math ops */

ccl_device_inline float safe_sqrtf(const float f)
{
  return sqrtf(max(f, 0.0f));
}

ccl_device_inline float inversesqrtf(const float f)
{
  return (f > 0.0f) ? 1.0f / sqrtf(f) : 0.0f;
}

ccl_device float safe_asinf(const float a)
{
  return asinf(clamp(a, -1.0f, 1.0f));
}

ccl_device float safe_acosf(const float a)
{
  return acosf(clamp(a, -1.0f, 1.0f));
}

ccl_device float compatible_powf(const float x, const float y)
{
#ifdef __KERNEL_COMPUTE__
  if (y == 0.0f) /* x^0 -> 1, including 0^0 */
    return 1.0f;

  /* GPU pow doesn't accept negative x, do manual checks here */
  if (x < 0.0f) {
    if (fmodf(-y, 2.0f) == 0.0f)
      return powf(-x, y);
    else
      return -powf(-x, y);
  }
  else if (x == 0.0f)
    return 0.0f;
#endif
  return powf(x, y);
}

ccl_device float safe_powf(const float a, const float b)
{
  if (UNLIKELY(a < 0.0f && b != float_to_int(b)))
    return 0.0f;

  return compatible_powf(a, b);
}

ccl_device float safe_divide(const float a, const float b)
{
  return (b != 0.0f) ? a / b : 0.0f;
}

ccl_device float safe_logf(const float a, const float b)
{
  if (UNLIKELY(a <= 0.0f || b <= 0.0f))
    return 0.0f;

  return safe_divide(logf(a), logf(b));
}

ccl_device float safe_modulo(const float a, const float b)
{
  return (b != 0.0f) ? fmodf(a, b) : 0.0f;
}

ccl_device_inline float sqr(const float a)
{
  return a * a;
}

ccl_device_inline float pow20(const float a)
{
  return sqr(sqr(sqr(sqr(a)) * a));
}

ccl_device_inline float pow22(float a)
{
  return sqr(a * sqr(sqr(sqr(a)) * a));
}

ccl_device_inline float beta(float x, float y)
{
#ifndef __KERNEL_OPENCL__
  return expf(lgammaf(x) + lgammaf(y) - lgammaf(x + y));
#else
  return expf(lgamma(x) + lgamma(y) - lgamma(x + y));
#endif
}

ccl_device_inline float xor_signmask(const float x, const int y)
{
  return __int_as_float(__float_as_int(x) ^ y);
}

ccl_device float bits_to_01(const uint bits)
{
  return bits * (1.0f / (float)0xFFFFFFFF);
}

ccl_device_inline uint count_leading_zeros(const uint x)
{
#if defined(__KERNEL_CUDA__) || defined(__KERNEL_OPTIX__)
  return __clz(x);
#elif defined(__KERNEL_OPENCL__)
  return clz(x);
#else
  assert(x != 0);
#  ifdef _MSC_VER
  unsigned long leading_zero = 0;
  _BitScanReverse(&leading_zero, x);
  return (31 - leading_zero);
#  else
  return __builtin_clz(x);
#  endif
#endif
}

ccl_device_inline uint count_trailing_zeros(const uint x)
{
#if defined(__KERNEL_CUDA__) || defined(__KERNEL_OPTIX__)
  return (__ffs(x) - 1);
#elif defined(__KERNEL_OPENCL__)
  return (31 - count_leading_zeros(x & -x));
#else
  assert(x != 0);
#  ifdef _MSC_VER
  unsigned long ctz = 0;
  _BitScanForward(&ctz, x);
  return ctz;
#  else
  return __builtin_ctz(x);
#  endif
#endif
}

ccl_device_inline uint find_first_set(const uint x)
{
#if defined(__KERNEL_CUDA__) || defined(__KERNEL_OPTIX__)
  return __ffs(x);
#elif defined(__KERNEL_OPENCL__)
  return (x != 0) ? (32 - count_leading_zeros(x & (-x))) : 0;
#else
#  ifdef _MSC_VER
  return (x != 0) ? (32 - count_leading_zeros(x & (-x))) : 0;
#  else
  return __builtin_ffs(x);
#  endif
#endif
}

/* projections */
ccl_device_inline float2 map_to_tube(const float3 co)
{
  float len, u, v;
  len = sqrtf(co.x * co.x + co.y * co.y);
  if (len > 0.0f) {
    u = (1.0f - (atan2f(co.x / len, co.y / len) / M_PI_F)) * 0.5f;
    v = (co.z + 1.0f) * 0.5f;
  }
  else {
    u = v = 0.0f;
  }
  return make_float2(u, v);
}

ccl_device_inline float2 map_to_sphere(const float3 co)
{
  float l = length(co);
  float u, v;
  if (l > 0.0f) {
    if (UNLIKELY(co.x == 0.0f && co.y == 0.0f)) {
      u = 0.0f; /* othwise domain error */
    }
    else {
      u = (1.0f - atan2f(co.x, co.y) / M_PI_F) / 2.0f;
    }
    v = 1.0f - safe_acosf(co.z / l) / M_PI_F;
  }
  else {
    u = v = 0.0f;
  }
  return make_float2(u, v);
}

/* Compares two floats.
 * Returns true if their absolute difference is smaller than abs_diff (for numbers near zero)
 * or their relative difference is less than ulp_diff ULPs.
 * Based on
 * https://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
 */

ccl_device_inline float compare_floats(const float a,
                                       const float b,
                                       const float abs_diff,
                                       const int ulp_diff)
{
  if (fabsf(a - b) < abs_diff) {
    return true;
  }

  if ((a < 0.0f) != (b < 0.0f)) {
    return false;
  }

  return (abs(__float_as_int(a) - __float_as_int(b)) < ulp_diff);
}

ccl_device_inline float clamp_to_normal(const float a)
{
  return fminf(fmaxf(a, 0.0f), 1.0f);
}

ccl_device_inline float4 interp_f4f4(const float4 a, const float4 b, const float t)
{
  const float s = 1.0f - t;

  return s * a + t * b;
}

CCL_NAMESPACE_END

#endif
