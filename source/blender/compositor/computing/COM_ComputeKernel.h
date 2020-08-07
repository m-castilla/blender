#ifndef __COM_COMPUTEKERNEL_H__
#define __COM_COMPUTEKERNEL_H__

#include <string>

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
  virtual void addIntArg(int value) = 0;
  virtual void addBoolArg(bool value) = 0;
  virtual void addFloatArg(float value) = 0;
  virtual void addFloat2Arg(float *value) = 0;
  virtual void addFloat3Arg(float *value) = 0;
  virtual void addFloat4Arg(float *value) = 0;
  // Add constant float4 (read only) array
  virtual void addFloat4CArrayArg(float *value, int n_elems) = 0;
  // Add constant int (read only) array
  virtual void addIntCArrayArg(int *value, int n_elems) = 0;
  // Add constant float (read only) array
  virtual void addFloatCArrayArg(float *float_array, int n_elems) = 0;
  virtual bool hasWorkEnqueued() = 0;

 protected:
  ComputeKernel(std::string kernel_name) : m_kernel_name(kernel_name)
  {
  }
};

#endif
