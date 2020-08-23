#ifndef __COM_KERNEL_CPU_NOCOMPAT_H__
#define __COM_KERNEL_CPU_NOCOMPAT_H__

//#include "COM_kernel_cpu.h"
//
//#define CPU_READ_DECL(src) \
//  PixelsImg src##_img = src->pixelsImg(); \
//  size_t src##_offset; \
//  (void)src##_offset;
//
//#define CPU_WRITE_DECL(dst) \
//  PixelsImg dst##_img = dst.pixelsImg(); \
//  int dst##_start_x = dst##_img.start_x; \
//  int dst##_start_y = dst##_img.start_y; \
//  int write_offset_x, write_offset_y; \
//  size_t dst##_offset; \
//  (void)dst##_offset;
//
//#define CPU_READ_OFFSET(src, dst) \
//  src##_offset = src##_img.is_single_elem ? \
//                     0 : \
//                     (src##_img.brow_chs * ((size_t)dst##_start_y + write_offset_y) + \
//                      ((size_t)dst##_start_x + write_offset_x) * (size_t)src##_img.elem_chs);
//#define CPU_WRITE_OFFSET(dst) \
//  dst##_offset = dst##_img.brow_chs * ((size_t)dst##_start_y + write_offset_y) + \
//                 ((size_t)dst##_start_x + write_offset_x) * dst##_img.elem_chs;
//
//#define CPU_READ_IMG(src, offset, result) \
//  if (src##_img.is_single_elem) { \
//    switch (src##_img.elem_chs) { \
//      case 4: \
//        result.w = src##_img.buffer[3]; \
//      case 3: \
//        result.z = src##_img.buffer[2]; \
//        result.y = src##_img.buffer[1]; \
//      case 1: \
//        result.x = src##_img.buffer[0]; \
//        break; \
//      default: \
//        kernel_assert(!"invalid elem_chs"); \
//        break; \
//    } \
//  } \
//  else { \
//    switch (src##_img.elem_chs) { \
//      case 4: \
//        result.w = src##_img.buffer[offset + 3]; \
//      case 3: \
//        result.z = src##_img.buffer[offset + 2]; \
//        result.y = src##_img.buffer[offset + 1]; \
//      case 1: \
//        result.x = src##_img.buffer[0]; \
//        break; \
//      default: \
//        kernel_assert(!"invalid elem_chs"); \
//        break; \
//    } \
//  }
//
//#define CPU_WRITE_IMG(dst, offset, pixel) \
//  switch (dst##_img.elem_chs) { \
//    case 4: \
//      dst##_img.buffer[offset + 3] = pixel.w; \
//    case 3: \
//      dst##_img.buffer[offset + 2] = pixel.z; \
//      dst##_img.buffer[offset + 1] = pixel.y; \
//    case 1: \
//      dst##_img.buffer[offset] = pixel.x; \
//      break; \
//    default: \
//      kernel_assert(!"invalid elem_chs"); \
//      break; \
//  }

#endif
