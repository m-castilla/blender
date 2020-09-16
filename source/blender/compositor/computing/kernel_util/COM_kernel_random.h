#ifndef __COM_KERNEL_RANDOM_H__
#define __COM_KERNEL_RANDOM_H__

#include "kernel_util/COM_kernel_math.h"

/* Implementation taken from  BLI_rand.hh */
CCL_NAMESPACE_BEGIN

ccl_constant uint64_t multiplier = 0x5DEECE66Dll;
ccl_constant uint64_t addend = 0xB;
ccl_constant uint64_t mask = 0x0000FFFFFFFFFFFFll;

ccl_device_inline float random_float(uint64_t *state_)
{
  *state_ = (multiplier * (*state_) + addend) & mask;
  return (float)(long)((*state_) >> 17) / 0x80000000;
}

ccl_device_inline int random_int(uint64_t *state_)
{
  *state_ = (multiplier * (*state_) + addend) & mask;
  return (int)((*state_) >> 17);
}

CCL_NAMESPACE_END

#endif
