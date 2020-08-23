/*
 * Copyright 2011-2017 Blender Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __COM_KERNEL_TYPES_FLOAT4_IMPL_H__
#define __COM_KERNEL_TYPES_FLOAT4_IMPL_H__

#ifndef __COM_KERNEL_TYPES_H__
#  error "Do not include this file directly, include COM_kernel_types.h instead."
#endif

CCL_NAMESPACE_BEGIN

#ifdef __KERNEL_SSE__
__forceinline float4::float4()
{
}

__forceinline float4::float4(const __m128 &a) : m128(a)
{
}

__forceinline float4::operator const __m128 &() const
{
  return m128;
}

__forceinline float4::operator __m128 &()
{
  return m128;
}

__forceinline float4 &float4::operator=(const float4 &a)
{
  m128 = a.m128;
  return *this;
}
#endif /* __KERNEL_SSE__ */

__forceinline float float4::operator[](int i) const
{
  kernel_assert(i >= 0);
  kernel_assert(i < 4);
  return *(&x + i);
}

__forceinline float &float4::operator[](int i)
{
  kernel_assert(i >= 0);
  kernel_assert(i < 4);
  return *(&x + i);
}

ccl_device_inline float4 make_float4_1(float f)
{
#ifdef __KERNEL_SSE__
  float4 a(_mm_set1_ps(f));
#else
  float4 a = {f, f, f, f};
#endif
  return a;
}

ccl_device_inline float4 make_float4(float x, float y, float z, float w)
{
#ifdef __KERNEL_SSE__
  float4 a(_mm_set_ps(w, z, y, x));
#else
  float4 a = {x, y, z, w};
#endif
  return a;
}

ccl_device_inline float4 make_float4_3(const float3 &f3)
{
#ifdef __KERNEL_SSE__
  float4 a(f3.m128);
#else
  float4 a = {f3.x, f3.y, f3.z, 0.0f};
#endif
  return a;
}

ccl_device_inline float4 make_float4_31(const float3 &f3, const float alpha)
{
#ifdef __KERNEL_SSE__
  float4 a(f3.m128);
  a.w = alpha;
#else
  float4 a = {f3.x, f3.y, f3.z, alpha};
#endif
  return a;
}

ccl_device_inline void print_float4(const char *label, const float4 &a)
{
  printf("%s: %.8f %.8f %.8f %.8f\n", label, (double)a.x, (double)a.y, (double)a.z, (double)a.w);
}

CCL_NAMESPACE_END

#endif
