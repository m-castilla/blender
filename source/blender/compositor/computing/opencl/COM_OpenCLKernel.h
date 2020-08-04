#ifndef __COM_OPENCLKERNEL_H__
#define __COM_OPENCLKERNEL_H__

#include "COM_ComputeKernel.h"
#include "clew.h"
#include <string>
#include <vector>

class PixelsRect;
struct PixelsSampler;
class OpenCLManager;
class OpenCLPlatform;
class OpenCLDevice;
class OpenCLKernel : public ComputeKernel {
 public:
  OpenCLKernel(OpenCLManager &man,
               OpenCLPlatform &platform,
               OpenCLDevice *device,
               std::string kernel_name,
               cl_kernel cl_kernel);
  ~OpenCLKernel();

  void initialize();

  cl_kernel getClKernel()
  {
    return m_cl_kernel;
  }
  void reset(ComputeDevice *new_device) override;
  void addReadImgArgs(PixelsRect &pixels) override;
  cl_mem addWriteImgArgs(PixelsRect &pixels);
  void addSamplerArg(PixelsSampler &sampler) override;
  void addIntArg(int value) override;
  void addBoolArg(bool value) override;
  void addFloatArg(float value) override;
  void addFloat2Arg(float *value) override;
  void addFloat3Arg(float *value) override;
  void addFloat4Arg(float *value) override;

  size_t getMaxGroupSize()
  {
    return m_max_group_size;
  }

  size_t getGroupSizeMultiple()
  {
    return m_group_size_multiple;
  }

 private:
  OpenCLPlatform &m_platform;
  OpenCLManager &m_man;
  OpenCLDevice *m_device;

  cl_kernel m_cl_kernel;
  size_t m_max_group_size;
  size_t m_group_size_multiple;
#if DEBUG
  size_t m_local_mem_used;
  size_t m_private_mem_used;
#endif
  bool m_initialized;
  int m_args_count;
};

#endif
