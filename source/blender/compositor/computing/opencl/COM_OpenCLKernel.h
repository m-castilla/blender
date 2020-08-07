#ifndef __COM_OPENCLKERNEL_H__
#define __COM_OPENCLKERNEL_H__

#include "COM_ComputeKernel.h"
#include "clew.h"
#include <functional>
#include <string>
#include <vector>

class PixelsRect;
struct PixelsSampler;
class OpenCLManager;
class OpenCLPlatform;
class OpenCLDevice;
class OpenCLKernel : public ComputeKernel {
 private:
  bool m_work_enqueued;
  OpenCLPlatform &m_platform;
  OpenCLManager &m_man;
  OpenCLDevice *m_device;
  std::vector<cl_mem> m_args_buffers;

  cl_kernel m_cl_kernel;
  size_t m_max_group_size;
  size_t m_group_size_multiple;
  bool m_initialized;
  int m_args_count;
#if DEBUG
  size_t m_local_mem_used;
  size_t m_private_mem_used;
#endif

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
  void clearArgs() override;
  void addReadImgArgs(PixelsRect &pixels) override;
  // returns a function to set the write image offset args. Call with (offset_x, offset_y)
  std::function<void(int, int)> addWriteImgArgs(PixelsRect &pixels);
  void addSamplerArg(PixelsSampler &sampler) override;
  void addIntArg(int value) override;
  void addBoolArg(bool value) override;
  void addFloatArg(float value) override;
  void addFloat2Arg(float *value) override;
  void addFloat3Arg(float *value) override;
  void addFloat4Arg(float *value) override;
  // Add float4 constant (read only) array
  void addFloat4CArrayArg(float *float4_array, int n_elems);
  // Add int constant (read only) array
  void addIntCArrayArg(int *int_array, int n_elems);
  void addFloatCArrayArg(float *float_array, int n_elems);
  bool hasWorkEnqueued() override
  {
    return m_work_enqueued;
  }

  size_t getMaxGroupSize()
  {
    return m_max_group_size;
  }

  size_t getGroupSizeMultiple()
  {
    return m_group_size_multiple;
  }

 private:
  cl_mem addReadOnlyBufferArg(void *data, size_t data_size);
};

#endif
