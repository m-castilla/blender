#ifndef __COM_COMPUTEKERNEL_H__
#define __COM_COMPUTEKERNEL_H__

#include <string>
#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif

#include "COM_Buffer.h"

#include "COM_kernel_cpu.h"

class PixelsRect;
struct PixelsSampler;
class ComputeDevice;
class ComputeKernel {
 private:
  std::string m_kernel_name;

 public:
  std::string getKernelName()
  {
    return m_kernel_name;
  }
  virtual void reset(ComputeDevice *new_device) = 0;
  virtual void clearArgs() = 0;
  virtual void addReadImgArgs(PixelsRect &pixels) = 0;
  virtual void addSamplerArg(PixelsSampler &sampler) = 0;
  virtual void addBoolArg(bool value) = 0;
  virtual void addIntArg(int value) = 0;
  virtual void addInt3Arg(const CCL::int3 &value) = 0;
  virtual void addFloatArg(float value) = 0;
  virtual void addFloat2Arg(const CCL::float2 &value) = 0;
  virtual void addFloat3Arg(const CCL::float3 &value) = 0;
  virtual void addFloat4Arg(const CCL::float4 &value) = 0;
  virtual void addUInt64Arg(const uint64_t value) = 0;
  // Parameter in kernel signature must be uint64_t
  virtual void addRandomSeedArg();

  virtual void addFloat3ArrayArg(const CCL::float3 *float3_array,
                                 int n_elems,
                                 MemoryAccess mem_access) = 0;
  virtual void addFloat4ArrayArg(const CCL::float4 *carray,
                                 int n_elems,
                                 MemoryAccess mem_access) = 0;
  virtual void addIntArrayArg(int *value, int n_elems, MemoryAccess mem_access) = 0;
  virtual void addFloatArrayArg(float *float_array, int n_elems, MemoryAccess mem_access) = 0;
  virtual void addUInt64ArrayArg(uint64_t *uint64_array, int n_elems, MemoryAccess mem_access) = 0;

  virtual bool hasWorkEnqueued() = 0;

  static uint64_t getRandomSeedArg(void *caller_object);

  virtual ~ComputeKernel();

 protected:
  ComputeKernel(std::string kernel_name);

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:TmpBuffer")
#endif
};

#endif
