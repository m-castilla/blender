#ifndef __COM_KERNEL_CPU_H__
#define __COM_KERNEL_CPU_H__

#define __KERNEL_CPU__

#define ccl_addr_space

/* Qualifiers for kernel code shared by CPU and compute managers */
#define ccl_kernel static void
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

#define CCL ccl
#define CCL_NAMESPACE_BEGIN namespace CCL {
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
#include "kernel_util/COM_kernel_sampling.h"
#include "kernel_util/COM_kernel_types.h"

/* Kernel function signature macros*/
#define CCL_READ(src) std::shared_ptr<PixelsRect> src
#define CCL_WRITE(dst) PixelsRect &dst
#define CCL_SAMPLER(sampler) const PixelsSampler sampler
/* END of OpenCL kernel function signature macros*/

#define READ_DECL(src) \
  PixelsImg src##_img = src->pixelsImg(); \
  CCL::float4 src##_pix; \
  size_t src##_offset; \
  CCL::int2 src##_coords; \
  CCL::float2 src##_coordsf; \
  (void)src##_pix; \
  (void)src##_offset; \
  (void)src##_coords; \
  (void)src##_coordsf;

#define WRITE_DECL(dst) \
  PixelsImg dst##_img = dst.pixelsImg(); \
  CCL::int2 dst##_coords = CCL::make_int2(dst##_img.start_x, dst##_img.start_y); \
  size_t dst##_offset; \
  kernel_assert(dst##_img.belem_chs_incr > 0 && dst##_img.brow_chs_incr > 0);

/*CPU op loop*/
#define CPU_LOOP_START(dst) \
  while (dst##_coords.y < dst##_img.end_y) { \
    dst##_offset = dst##_img.brow_chs_incr * dst##_coords.y + \
                   dst##_coords.x * dst##_img.belem_chs_incr; \
    while (dst##_coords.x < dst##_img.end_x) {

#define CPU_LOOP_END \
  ++dst##_coords.x; \
  dst##_offset += dst##_img.belem_chs_incr; \
  } \
  dst##_coords.x = dst##_img.start_x; \
  ++dst##_coords.y; \
  }
/*END of CPU op loop*/

#define SET_COORDS(src, x_, y_) \
  src##_coords.x = (x_); \
  src##_coords.y = (y_); \
  src##_offset = src##_img.brow_chs_incr * src##_coords.y + \
                 src##_coords.x * src##_img.belem_chs_incr;

#define SET_SAMPLE_COORDS(src, x_, y_) \
  src##_coordsf.x = (x_); \
  src##_coordsf.y = (y_);

#define COPY_COORDS(to, coords) \
  to##_coords = coords; \
  to##_offset = to##_img.brow_chs_incr * to##_coords.y + to##_coords.x * to##_img.belem_chs_incr;

#define COPY_SAMPLE_COORDS(to, coords) to##_coordsf = coords;

#define UPDATE_COORDS_X(src, x_) \
  src##_offset += (x_ - (size_t)src##_coords.x) * src##_img.belem_chs_incr; \
  src##_coords.x = x_; \
  kernel_assert(src##_offset == src##_img.brow_chs_incr * src##_coords.y + \
                                    src##_coords.x * src##_img.belem_chs_incr);

#define UPDATE_SAMPLE_COORDS_X(src, x_) src##_coordsf.x = x_;

#define UPDATE_COORDS_Y(src, y_) \
  src##_offset += (y_ - (size_t)src##_coords.y) * src##_img.brow_chs_incr; \
  src##_coords.y = y_; \
  kernel_assert(src##_offset == src##_img.brow_chs_incr * src##_coords.y + \
                                    src##_coords.x * src##_img.belem_chs_incr);

#define UPDATE_SAMPLE_COORDS_Y(src, y_) src##_coordsf.y = y_;

#define INCR1_COORDS_X(src) \
  src##_offset += src##_img.belem_chs_incr; \
  src##_coords.x++; \
  kernel_assert(src##_offset == src##_img.brow_chs_incr * src##_coords.y + \
                                    src##_coords.x * src##_img.belem_chs_incr);

#define INCR1_SAMPLE_COORDS_X(src) src##_coordsf.x++;

#define INCR1_COORDS_Y(src) \
  src##_offset += src##_img.brow_chs_incr; \
  src##_coords.y++; \
  kernel_assert(src##_offset == src##_img.brow_chs_incr * src##_coords.y + \
                                    src##_coords.x * src##_img.belem_chs_incr);

#define INCR1_SAMPLE_COORDS_Y(src) src##_coordsf.y++;

#define DECR1_COORDS_X(src) \
  src##_offset -= src##_img.belem_chs_incr; \
  src##_coords.x--; \
  kernel_assert(src##_coords.x >= src##_img.start_x && src##_coords.x < src##_img.end_x); \
  kernel_assert(src##_offset == src##_img.brow_chs_incr * src##_coords.y + \
                                    src##_coords.x * src##_img.belem_chs_incr);

#define DECR1_SAMPLE_COORDS_X(src) src##_coordsf.x--;

#define DECR1_COORDS_Y(src) \
  src##_offset -= src##_img.brow_chs_incr; \
  src##_coords.y--; \
  kernel_assert(src##_offset == src##_img.brow_chs_incr * src##_coords.y + \
                                    src##_coords.x * src##_img.belem_chs_incr);

#define DECR1_SAMPLE_COORDS_Y(src) src##_coordsf.y--;

#define INCR_COORDS_X(src, incr) \
  src##_offset += src##_img.belem_chs_incr * incr; \
  src##_coords.x += incr; \
  kernel_assert(src##_offset == src##_img.brow_chs_incr * src##_coords.y + \
                                    src##_coords.x * src##_img.belem_chs_incr);

#define INCR_SAMPLE_COORDS_X(src, incr) src##_coordsf.x += incr;

#define INCR_COORDS_Y(src, incr) \
  src##_offset += src##_img.brow_chs_incr * incr; \
  src##_coords.y += incr; \
  kernel_assert(src##_offset == src##_img.brow_chs_incr * src##_coords.y + \
                                    src##_coords.x * src##_img.belem_chs_incr);

#define INCR_SAMPLE_COORDS_Y(src, incr) src##_coordsf.y += incr;

#define READ_IMG1(src, result) \
  result.x = src##_img.buffer[src##_offset]; \
  kernel_assert(src##_coords.x >= src##_img.start_x && src##_coords.x < src##_img.end_x); \
  kernel_assert(src##_coords.y >= src##_img.start_y && src##_coords.y < src##_img.end_y); \
  kernel_assert(src##_offset >= (src##_img.brow_chs_incr * src##_img.start_y + \
                                 src##_img.start_x * src##_img.belem_chs_incr) && \
                src##_offset <= (src##_img.brow_chs_incr * ((size_t)src##_img.end_y - 1) + \
                                 ((size_t)src##_img.end_x - 1) * src##_img.belem_chs_incr));

#define READ_IMG3(src, result) \
  result = CCL::make_float4(src##_img.buffer[src##_offset], \
                            src##_img.buffer[src##_offset + 1], \
                            src##_img.buffer[src##_offset + 2], \
                            0.0f); \
  kernel_assert(src##_coords.x >= src##_img.start_x && src##_coords.x < src##_img.end_x); \
  kernel_assert(src##_coords.y >= src##_img.start_y && src##_coords.y < src##_img.end_y); \
  kernel_assert(src##_offset >= (src##_img.brow_chs_incr * src##_img.start_y + \
                                 src##_img.start_x * src##_img.belem_chs_incr) && \
                src##_offset <= (src##_img.brow_chs_incr * ((size_t)src##_img.end_y - 1) + \
                                 ((size_t)src##_img.end_x - 1) * src##_img.belem_chs_incr));

#define READ_IMG4(src, result) \
  result = CCL::make_float4(src##_img.buffer[src##_offset], \
                            src##_img.buffer[src##_offset + 1], \
                            src##_img.buffer[src##_offset + 2], \
                            src##_img.buffer[src##_offset + 3]); \
  kernel_assert(src##_coords.x >= src##_img.start_x && src##_coords.x < src##_img.end_x); \
  kernel_assert(src##_coords.y >= src##_img.start_y && src##_coords.y < src##_img.end_y); \
  kernel_assert(src##_offset >= (src##_img.brow_chs_incr * src##_img.start_y + \
                                 src##_img.start_x * src##_img.belem_chs_incr) && \
                src##_offset <= (src##_img.brow_chs_incr * ((size_t)src##_img.end_y - 1) + \
                                 ((size_t)src##_img.end_x - 1) * src##_img.belem_chs_incr));

#define WRITE_IMG1(dst, pixel) \
  dst##_img.buffer[dst##_offset] = pixel.x; \
  kernel_assert(dst##_coords.x >= dst##_img.start_x && dst##_coords.x < dst##_img.end_x); \
  kernel_assert(dst##_coords.y >= dst##_img.start_y && dst##_coords.y < dst##_img.end_y); \
  kernel_assert(dst##_offset >= (dst##_img.brow_chs_incr * dst##_img.start_y + \
                                 dst##_img.start_x * dst##_img.belem_chs_incr) && \
                dst##_offset <= (dst##_img.brow_chs_incr * ((size_t)dst##_img.end_y - 1) + \
                                 ((size_t)dst##_img.end_x - 1) * dst##_img.belem_chs_incr));
#define WRITE_IMG3(dst, pixel) \
  dst##_img.buffer[dst##_offset] = pixel.x; \
  dst##_img.buffer[dst##_offset + 1] = pixel.y; \
  dst##_img.buffer[dst##_offset + 2] = pixel.z; \
  kernel_assert(dst##_coords.x >= dst##_img.start_x && dst##_coords.x < dst##_img.end_x); \
  kernel_assert(dst##_coords.y >= dst##_img.start_y && dst##_coords.y < dst##_img.end_y); \
  kernel_assert(dst##_offset >= (dst##_img.brow_chs_incr * dst##_img.start_y + \
                                 dst##_img.start_x * dst##_img.belem_chs_incr) && \
                dst##_offset <= (dst##_img.brow_chs_incr * ((size_t)dst##_img.end_y - 1) + \
                                 ((size_t)dst##_img.end_x - 1) * dst##_img.belem_chs_incr));

#ifdef __KERNEL_SSE__
#  define WRITE_IMG4(dst, pixel) \
    _mm_storeu_ps(&dst##_img.buffer[dst##_offset], pixel.m128); \
    kernel_assert(dst##_coords.x >= dst##_img.start_x && dst##_coords.x < dst##_img.end_x); \
    kernel_assert(dst##_coords.y >= dst##_img.start_y && dst##_coords.y < dst##_img.end_y); \
    kernel_assert(dst##_offset >= (dst##_img.brow_chs_incr * dst##_img.start_y + \
                                   dst##_img.start_x * dst##_img.belem_chs_incr) && \
                  dst##_offset <= (dst##_img.brow_chs_incr * ((size_t)dst##_img.end_y - 1) + \
                                   ((size_t)dst##_img.end_x - 1) * dst##_img.belem_chs_incr));
#else
#  define WRITE_IMG4(dst, pixel) \
    dst##_img.buffer[dst##_offset] = pixel.x; \
    dst##_img.buffer[dst##_offset + 1] = pixel.y; \
    dst##_img.buffer[dst##_offset + 2] = pixel.z; \
    dst##_img.buffer[dst##_offset + 3] = pixel.w; \
    kernel_assert(dst##_coords.x >= dst##_img.start_x && dst##_coords.x < dst##_img.end_x); \
    kernel_assert(dst##_coords.y >= dst##_img.start_y && dst##_coords.y < dst##_img.end_y); \
    kernel_assert(dst##_offset >= (dst##_img.brow_chs_incr * dst##_img.start_y + \
                                   dst##_img.start_x * dst##_img.belem_chs_incr) && \
                  dst##_offset <= (dst##_img.brow_chs_incr * ((size_t)dst##_img.end_y - 1) + \
                                   ((size_t)dst##_img.end_x - 1) * dst##_img.belem_chs_incr));
#endif

#define READ_IMG(src, pixel) \
  if (src##_img.elem_chs == 4) { \
    READ_IMG4(src, pixel); \
  } \
  else if (src##_img.elem_chs == 1) { \
    READ_IMG1(src, pixel); \
  } \
  else { \
    READ_IMG3(src, pixel); \
  }

#define WRITE_IMG(dst, pixel) \
  if (dst##_img.elem_chs == 4) { \
    WRITE_IMG4(dst, pixel); \
  } \
  else if (dst##_img.elem_chs == 1) { \
    WRITE_IMG1(dst, pixel); \
  } \
  else { \
    WRITE_IMG3(dst, pixel); \
  }

#include "kernel_util/COM_kernel_sampling.h"

#define SAMPLE_IMG(src, sampler, result) CCL::sample(src##_img, result, sampler, src##_coordsf);

#endif
