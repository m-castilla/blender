
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

/* clang-format off */

/* #define __forceinline triggers a bug in some clang-format versions, disable
 * format for entire file to keep results consistent. */

#ifndef __COM_KERNEL_DEFINES_H__
#define __COM_KERNEL_DEFINES_H__

/* On x86_64, versions of glibc < 2.16 have an issue where expf is
 * much slower than the double version.  This was fixed in glibc 2.16.
 */
#if defined(__x86_64__) && defined(__x86_64__) && defined(__GNU_LIBRARY__) && \
    defined(__GLIBC__) && defined(__GLIBC_MINOR__) && (__GLIBC__ <= 2 && __GLIBC_MINOR__ < 16)
#  define expf(x) ((float)exp((double)(x)))
#endif

/* Bitness */

#if defined(__ppc64__) || defined(__PPC64__) || defined(__x86_64__) || defined(__ia64__) || \
    defined(_M_X64)
#  define __KERNEL_64_BIT__
#endif

/* Use to suppress '-Wimplicit-fallthrough' (in place of 'break'). */
#ifndef ATTR_FALLTHROUGH
#  if defined(__GNUC__) && (__GNUC__ >= 7) /* gcc7.0+ only */
#    define ATTR_FALLTHROUGH __attribute__((fallthrough))
#  else
#    define ATTR_FALLTHROUGH ((void)0)
#  endif
#endif

/* macros */

/* hints for branch prediction, only use in code that runs a _lot_ */
#if defined(__GNUC__) && defined(__KERNEL_CPU__)
#  define LIKELY(x) __builtin_expect(!!(x), 1)
#  define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#  define LIKELY(x) (x)
#  define UNLIKELY(x) (x)
#endif

#if defined(__GNUC__) || defined(__clang__)
#  define UNUSED(x) UNUSED_##x __attribute__((__unused__))
#else
#  define UNUSED(x) UNUSED_##x
#endif

#if defined(__GNUC__) || defined(__clang__)
#  if defined(__cplusplus)
/* Some magic to be sure we don't have reference in the type. */
template<typename T> static inline T decltype_helper(T x)
{
  return x;
}
#    define TYPEOF(x) decltype(decltype_helper(x))
#  else
#    define TYPEOF(x) typeof(x)
#  endif
#endif

/* Causes warning:
 * incompatible types when assigning to type 'Foo' from type 'Bar'
 * ... the compiler optimizes away the temp var */
#undef CHECK_TYPE
#undef CHECK_TYPE_PAIR
#undef CHECK_TYPE_INLINE

#if defined(__GNUC__) && !defined(__KERNEL_COMPUTE__)
#  define CHECK_TYPE(var, type) \
    { \
      TYPEOF(var) * __tmp; \
      __tmp = (type *)NULL; \
      (void)__tmp; \
    } \
    (void)0

#  define CHECK_TYPE_PAIR(var_a, var_b) \
    { \
      TYPEOF(var_a) * __tmp; \
      __tmp = (typeof(var_b) *)NULL; \
      (void)__tmp; \
    } \
    (void)0
#else
#  define CHECK_TYPE(var, type)
#  define CHECK_TYPE_PAIR(var_a, var_b)
#endif

/* can be used in simple macros */
#if !defined(__KERNEL_COMPUTE__)
#define CHECK_TYPE_INLINE(val, type) ((void)(((type)0) != (val)))
#else
#  define CHECK_TYPE_INLINE(val, type)
#endif

#undef SWAP
#define SWAP(type, a, b) \
  { \
    type sw_ap; \
    CHECK_TYPE(a, type); \
    CHECK_TYPE(b, type); \
    sw_ap = (a); \
    (a) = (b); \
    (b) = sw_ap; \
  } \
  (void)0

#endif /* __COM_KERNEL_DEFINES_H__ */
