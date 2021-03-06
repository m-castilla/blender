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

#ifndef __COM_KERNEL_TYPES_FLOAT4_H__
#define __COM_KERNEL_TYPES_FLOAT4_H__

#ifndef __COM_KERNEL_TYPES_H__
#  error "Do not include this file directly, include COM_kernel_types.h instead."
#endif

CCL_NAMESPACE_BEGIN

struct int4;

struct ccl_try_align(16) float4
{
#ifdef __KERNEL_SSE__
#  pragma warning(push)
#  pragma warning(disable : 4201)
  union {
    __m128 m128;
    struct {
      float x, y, z, w;
    };
  };
#  pragma warning(pop)

  __forceinline float4();
  __forceinline explicit float4(const __m128 &a);

  __forceinline operator const __m128 &() const;
  __forceinline operator __m128 &();

  __forceinline float4 &operator=(const float4 &a);

#else  /* __KERNEL_SSE__ */
  float x, y, z, w;
#endif /* __KERNEL_SSE__ */
};

ccl_device_inline float4 make_float4_1(float f);
ccl_device_inline float4 make_float4(float x, float y, float z, float w);
ccl_device_inline float4 make_float4_3(const float3 &f3);
ccl_device_inline float4 make_float4_31(const float3 &f3, const float alpha);
ccl_device_inline float4 make_float4_a(const float *array_);
ccl_device_inline void print_float4(const char *label, const float4 &a);

CCL_NAMESPACE_END

#endif
