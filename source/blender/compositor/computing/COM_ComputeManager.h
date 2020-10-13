#ifndef __COM_COMPUTEMANAGER_H__
#define __COM_COMPUTEMANAGER_H__

#include "MEM_guardedalloc.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

enum class ComputeType { NONE, OPENCL };
class PixelsRect;
struct PixelsSampler;
class ComputeKernel;
class ComputeDevice;
class ExecutionManager;
class ComputeManager {
 private:
  ComputeDevice *m_device;
  bool m_is_initialized;

 public:
  virtual void initialize();
  bool canCompute() const
  {
    return m_device != nullptr;
  }
  bool isInitialized()
  {
    return m_is_initialized;
  }
  ComputeDevice *getSelectedDevice()
  {
    return m_device;
  }
  int getNComputeUnits();
  int getMaxImgW();
  int getMaxImgH();

  virtual ComputeType getComputeType() = 0;

  virtual ~ComputeManager() = 0;

 protected:
  ComputeManager();

  /* Returns {source code, source path} */
  std::pair<std::string, std::string> loadKernelsSource();
  virtual std::vector<ComputeDevice *> getDevices() = 0;

 private:
  void assertKernelsSync();

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:ComputeManager")
#endif
};

#endif
