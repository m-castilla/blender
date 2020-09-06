#ifndef __COM_COMPUTEDEVICE_H__
#define __COM_COMPUTEDEVICE_H__

#include "COM_Buffer.h"
#include "MEM_guardedalloc.h"
#include <functional>
#include <string>
#include <unordered_map>

struct PixelsSampler;
class PixelsRect;
class ComputeKernel;

enum class ComputeDeviceType { CPU, GPU };
class ExecutionManager;
class ComputeDevice {
 private:
  bool m_initialized;

 public:
  virtual void initialize()
  {
    m_initialized = true;
  }
  bool isInitialized()
  {
    return m_initialized;
  }

  virtual void queueJob(PixelsRect &dst,
                        std::string kernel_name,
                        std::function<void(ComputeKernel *)> add_kernel_args_func) = 0;
  virtual void waitQueueToFinish() = 0;

  virtual void *memDeviceAlloc(MemoryAccess mem_access,
                               int width,
                               int height,
                               bool alloc_for_host_map) = 0;
  virtual float *memDeviceToHostMapEnqueue(void *device_buffer,
                                           MemoryAccess mem_access,
                                           int width,
                                           int height,
                                           size_t &r_map_row_pitch) = 0;
  virtual void memDeviceToHostUnmapEnqueue(void *device_buffer, float *host_mapped_buffer) = 0;
  virtual void memDeviceToHostCopyEnqueue(
      float *r_host_buffer, void *device_buffer, size_t host_row_bytes, int width, int height) = 0;
  virtual void memDeviceFree(void *device_buffer) = 0;
  virtual void memHostToDeviceCopyEnqueue(
      void *r_device_buffer, float *host_buffer, size_t host_row_bytes, int width, int height) = 0;

  virtual ComputeDeviceType getDeviceType() = 0;
  virtual int getNComputeUnits() = 0;
  virtual int getMaxImgW() = 0;
  virtual int getMaxImgH() = 0;

 protected:
  ComputeDevice();
  virtual ~ComputeDevice();

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:ComputeDevice")
#endif
};

#endif
