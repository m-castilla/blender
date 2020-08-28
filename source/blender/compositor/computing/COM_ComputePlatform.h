#ifndef __COM_COMPUTEPLATFORM_H__
#define __COM_COMPUTEPLATFORM_H__

#include "COM_Pixels.h"
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>
#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif

class ComputeKernel;
class ExecutionManager;
class ComputeDevice;
struct PixelsSampler;
class ComputePlatform {
 private:
  std::unordered_map<std::string, std::vector<ComputeKernel *>> m_kernels;
  std::unordered_map<PixelsSampler, void *> m_samplers;
  int m_kernels_count;
  bool m_is_initialized;
  bool m_samplers_deleted;

 public:
  void initialize();

  /* recycleKernel must be called after finished using the kernel */
  ComputeKernel *getKernel(std::string kernel_name, ComputeDevice *device);
  void recycleKernel(ComputeKernel *kernel);
  void *getSampler(PixelsSampler &pix_sampler);

  ~ComputePlatform();

 protected:
  ComputePlatform();
  virtual ComputeKernel *createKernel(std::string kernel_name, ComputeDevice *device) = 0;
  virtual void *createSampler(PixelsSampler pix_sampler) = 0;

  /* Must be called in subclasses destructor */
  void deleteSamplers(std::function<void(void *)> deleteFunc);

 private:
  void cleanKernelsCache();

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:ComputePlatform")
#endif
};

#endif
