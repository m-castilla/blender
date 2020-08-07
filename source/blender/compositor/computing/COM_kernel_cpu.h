#ifndef __COM_KERNEL_CPU_H__
#define __COM_KERNEL_CPU_H__

#define __KERNEL_CPU__

#define ccl_addr_space

/* Qualifiers for kernel code shared by CPU and compute managers */
#define ccl_kernel void
#define ccl_device static inline
#define ccl_device_noinline static
#define ccl_device_noinline_cpu ccl_device_noinline
#define ccl_global
#define ccl_static_constant static const
#define ccl_constant const
#define ccl_local
#define ccl_local_param
#define ccl_private
#define ccl_restrict __restrict
#define ccl_ref &
#define ccl_optional_struct_init
#define ccl_loop_no_unroll
#define __KERNEL_WITH_SSE_ALIGN__

#if defined(_WIN32) && !defined(FREE_WINDOWS)
#  define ccl_device_inline static __forceinline
#  define ccl_device_forceinline static __forceinline
#  define ccl_align(...) __declspec(align(__VA_ARGS__))
#  ifdef __KERNEL_64_BIT__
#    define ccl_try_align(...) __declspec(align(__VA_ARGS__))
#  else /* __KERNEL_64_BIT__ */
#    undef __KERNEL_WITH_SSE_ALIGN__
/* No support for function arguments (error C2719). */
#    define ccl_try_align(...)
#  endif /* __KERNEL_64_BIT__ */
#  define ccl_may_alias
#  define ccl_always_inline __forceinline
#  define ccl_never_inline __declspec(noinline)
#  define ccl_maybe_unused
#else /* _WIN32 && !FREE_WINDOWS */
#  define ccl_device_inline static inline __attribute__((always_inline))
#  define ccl_device_forceinline static inline __attribute__((always_inline))
#  define ccl_align(...) __attribute__((aligned(__VA_ARGS__)))
#  ifndef FREE_WINDOWS64
#    define __forceinline inline __attribute__((always_inline))
#  endif
#  define ccl_try_align(...) __attribute__((aligned(__VA_ARGS__)))
#  define ccl_may_alias __attribute__((__may_alias__))
#  define ccl_always_inline __attribute__((always_inline))
#  define ccl_never_inline __attribute__((noinline))
#  define ccl_maybe_unused __attribute__((used))
#endif /* _WIN32 && !FREE_WINDOWS */

#define CCL_NAMESPACE ccl
#define CCL_NAMESPACE_BEGIN namespace CCL_NAMESPACE {
#define CCL_NAMESPACE_END }

CCL_NAMESPACE_BEGIN

/* Assertions inside the kernel only work for the CPU device, so we wrap it in
 * a macro which is empty for other devices */
#define kernel_assert(cond) assert(cond)

CCL_NAMESPACE_END
#include "kernel_util/COM_kernel_algo.h"
#include "kernel_util/COM_kernel_color.h"
#include "kernel_util/COM_kernel_defines.h"
#include "kernel_util/COM_kernel_geom.h"
#include "kernel_util/COM_kernel_math.h"
#include "kernel_util/COM_kernel_types.h"

CCL_NAMESPACE_BEGIN

/* Kernel function signature macros*/
#define CCL_READ(src) std::shared_ptr<PixelsRect> src
#define CCL_WRITE(dst) PixelsRect &dst
#define CCL_SAMPLER(sampler) PixelsSampler sampler
/* END of OpenCL kernel function signature macros*/

#define READ_DECL(src) \
  PixelsImg src##_img = src->pixelsImg(); \
  float4 src##_pix; \
  bool src##_single = src->is_single_elem;

#define SAMPLE_DECL(src) \
  PixelsImg src##_img = src->pixelsImg(); \
  float4 src##_pix; \
  bool src##_single = src->is_single_elem;

#define WRITE_DECL(dst) \
  PixelsImg dst##_img = dst.pixelsImg(); \
  int dst##_start_x = dst##_img.start_x; \
  int dst##_start_y = dst##_img.start_y; \
  int2 dst##_coords; \
  int write_offset_x, write_offset_y;

#define COORDS_TO_OFFSET(coords) \
  coords.x = dst##_start_x + write_offset_x; \
  coords.y = dst##_start_y + write_offset_y;

/*CPU op loop*/
#define CPU_LOOP_START(dst) \
  write_offset_x = 0; \
  write_offset_y = 0; \
  while (write_offset_y < dst##_img.col_elems) { \
    while (write_offset_x < dst##_img.row_elems) {

#define CPU_LOOP_END \
  ++write_offset_x; \
  } \
  write_offset_x = 0; \
  ++write_offset_y; \
  }
/*END of CPU op loop*/

/*Read pixel from image*/
#define READ_IMG(src, coords, result) \
  memcpy(&result, \
         (src##_single ? \
              src##_img.buffer : \
              &src##_img.buffer[src##_img.brow_chs * coords.y + coords.x * src##_img.elem_chs]), \
         src##_img.elem_bytes);

/* sampler must be a PixelsSampler, coords can be either float2 or int2, src_img may have any
 * number of channels*/
#define SAMPLE_IMG(src, coords, sampler, result) \
  if (src##_single) { \
    memcpy(&result, src##_img.buffer, src##_img.elem_bytes); \
  } \
  else { \
    src##_img.sample((float *)&result, sampler, coords.x, coords.y); \
  }

/* Write pixel to image */
#define WRITE_IMG(dst, coords, pixel) \
  memcpy(&dst##_img.buffer[dst##_img.brow_chs * coords.y + coords.x * dst##_img.elem_chs], \
         &pixel, \
         dst##_img.elem_bytes);

CCL_NAMESPACE_END

#endif
