#ifndef __COM_KERNEL_RANDOM_H__
#define __COM_KERNEL_RANDOM_H__

#include "kernel_util/COM_kernel_math.h"

/* Implementation taken from  BLI_rand.hh */
CCL_NAMESPACE_BEGIN

ccl_constant uint64_t multiplier = 0x5DEECE66Dll;
ccl_constant uint64_t addend = 0xB;
ccl_constant uint64_t mask = 0x0000FFFFFFFFFFFFll;

/*Thomas Wang's hash function. See:
 * http://web.archive.org/web/20071223173210/http://www.concentric.net/~Ttwang/tech/inthash.htm*/
ccl_device_inline uint64_t hash64shift(uint64_t key)
{
  key = (~key) + (key << 21);  // key = (key << 21) - key - 1;
  key = key ^ (key >> 24);
  key = (key + (key << 3)) + (key << 8);  // key * 265
  key = key ^ (key >> 14);
  key = (key + (key << 2)) + (key << 4);  // key * 21
  key = key ^ (key >> 28);
  key = key + (key << 31);
  return key;
}

ccl_device_inline int random_int(uint64_t *random_state,
                                 unsigned int *random_idx_state,
                                 int2 dst_coords)
{
#ifdef __KERNEL_COMPUTE__
  // The initial random state is the same for all work items. We hash it with the given item
  // dst_coords so that the seed is different for all work items. We must hash it always not only
  // once, otherwise it generates bad random numbers for noise effects
  uint64_t x_hash = hash64shift(*random_state ^ (*random_state * dst_coords.x));
  *random_state = hash64shift(x_hash ^ (*random_state * dst_coords.y));
#else
  (void)random_idx_state;
  (void)dst_coords;
#endif
  *random_state = (multiplier * (*random_state) + addend) & mask;

  return (int)((*random_state) >> 17);
}

ccl_device_inline float random_float(uint64_t *random_state,
                                     unsigned int *random_idx_state,
                                     int2 dst_coords)
{
  return (float)random_int(random_state, random_idx_state, dst_coords) / 0x80000000;
}

//
// ccl_device_inline int random_int(uint64_t *random_state, int2 dst_coords, int dst_width)
//{
//  uint64_t offset = (uint64_t)dst_coords.y * dst_width + dst_coords.x;
//  *random_state = (multiplier * (*random_state + offset) + addend) & mask;
//  return (int)((*random_state) >> 17);
//}

CCL_NAMESPACE_END

#endif
