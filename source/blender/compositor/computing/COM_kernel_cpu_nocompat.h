#ifndef __COM_KERNEL_CPU_NOCOMPAT_H__
#define __COM_KERNEL_CPU_NOCOMPAT_H__

#include "COM_kernel_cpu.h"

#define CPU_READ_DECL(src) \
  PixelsImg src##_img = src->pixelsImg(); \
  bool src##_single = src->is_single_elem; \
  int src##_offset; \
  (void)src##_offset;

#define CPU_WRITE_DECL(dst) \
  PixelsImg dst##_img = dst.pixelsImg(); \
  int dst##_start_x = dst##_img.start_x; \
  int dst##_start_y = dst##_img.start_y; \
  int write_offset_x, write_offset_y; \
  int dst##_offset; \
  (void)dst##_offset;

#define CPU_READ_OFFSET(src, dst) \
  src##_offset = src##_single ? 0 : \
                                (src##_img.brow_chs * (dst##_start_y + write_offset_y) + \
                                 (dst##_start_x + write_offset_x) * src##_img.elem_chs);
#define CPU_WRITE_OFFSET(dst) \
  dst##_offset = dst##_img.brow_chs * (dst##_start_y + write_offset_y) + \
                 (dst##_start_x + write_offset_x) * dst##_img.elem_chs;

#define CPU_READ_IMG(src, src_offset, pixel) \
  memcpy(&pixel, \
         src##_single ? src##_img.buffer : &src##_img.buffer[src_offset], \
         src##_img.elem_bytes);

#define CPU_WRITE_IMG(dst, dst_offset, pixel) \
  memcpy(&dst##_img.buffer[dst_offset], &pixel, dst##_img.elem_bytes);

#endif
