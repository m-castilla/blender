#ifndef __COM_KERNEL_BUFFER_H__
#define __COM_KERNEL_BUFFER_H__

#include "kernel_util/COM_kernel_types.h"

struct SimpleBuffer;
/* All this methods has been taken from blendlib math_geom.c file.
 * They are adapted for vectors and kernel compatibility */
CCL_NAMESPACE_BEGIN

typedef struct SimpleF4Buffer {
  float4 *buffer;
  size_t n_elems;
} SimpleF4Buffer;

static SimpleF4Buffer simple_buffer_to_f4(const SimpleBuffer &buffer);

CCL_NAMESPACE_END

#endif
