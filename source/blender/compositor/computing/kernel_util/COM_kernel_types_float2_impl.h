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

#ifndef __COM_KERNEL_TYPES_FLOAT2_IMPL_H__
#define __COM_KERNEL_TYPES_FLOAT2_IMPL_H__

#ifndef __COM_KERNEL_TYPES_H__
#  error "Do not include this file directly, include COM_kernel_types.h instead."
#endif

CCL_NAMESPACE_BEGIN

__forceinline float float2::operator[](int i) const
{
  kernel_assert(i >= 0);
  kernel_assert(i < 2);
  return *(&x + i);
}

__forceinline float &float2::operator[](int i)
{
  kernel_assert(i >= 0);
  kernel_assert(i < 2);
  return *(&x + i);
}

ccl_device_inline float2 make_float2(float x, float y)
{
  float2 a = {x, y};
  return a;
}

ccl_device_inline void print_float2(const char *label, const float2 &a)
{
  printf("%s: %.8f %.8f\n", label, (double)a.x, (double)a.y);
}

CCL_NAMESPACE_END

#endif
