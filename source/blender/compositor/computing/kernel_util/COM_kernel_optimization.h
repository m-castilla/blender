/*
 * Copyright 2011-2013 Blender Foundation
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

#ifndef __COM_KERNEL_OPTIMIZATION_H__
#define __COM_KERNEL_OPTIMIZATION_H__

#ifndef __COM_KERNEL_TYPES_H__
#  error "Do not include this file directly, include COM_kernel_types.h instead."
#endif

#ifndef __KERNEL_COMPUTE__

/* x86
 *
 * Compile a regular, SSE2 and SSE3 kernel. */

#  if defined(i386) || defined(_M_IX86)

/* We require minimum SSE2 support on x86, so auto enable. */
#    define __KERNEL_SSE__
#    define __KERNEL_SSE2__

#  endif /* defined(i386) || defined(_M_IX86) */

/* x86-64
 *
 * Compile a regular (includes SSE2), SSE3, SSE 4.1, AVX and AVX2 kernel. */

#  if defined(__x86_64__) || defined(_M_X64)

/* SSE2 is always available on x86-64 CPUs, so auto enable */
#    define __KERNEL_SSE__
#    define __KERNEL_SSE2__

#  endif /* defined(__x86_64__) || defined(_M_X64) */

#endif /* __KERNEL_COMPUTE__ */

#endif
