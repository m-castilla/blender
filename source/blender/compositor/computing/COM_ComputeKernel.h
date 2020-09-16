#ifndef __COM_COMPUTEKERNEL_H__
#define __COM_COMPUTEKERNEL_H__

#include <string>

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

  /* Constant read only array args */
  virtual void addFloat3CArrayArg(const CCL::float3 *float3_array, int n_elems) = 0;
  virtual void addFloat4CArrayArg(const CCL::float4 *carray, int n_elems) = 0;
  virtual void addIntCArrayArg(int *value, int n_elems) = 0;
  virtual void addFloatCArrayArg(float *float_array, int n_elems) = 0;
  /* */

  virtual bool hasWorkEnqueued() = 0;

  static uint64_t getRandomSeedArg(void *caller_object);

  virtual ~ComputeKernel();

 protected:
  ComputeKernel(std::string kernel_name);
};

#endif
