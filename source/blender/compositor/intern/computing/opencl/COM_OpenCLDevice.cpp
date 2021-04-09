#include "COM_OpenCLDevice.h"
#include "BLI_assert.h"
#include "BLI_utildefines.h"
#include "COM_OpenCLKernel.h"
#include "COM_OpenCLManager.h"
#include "COM_OpenCLPlatform.h"
#include "MEM_guardedalloc.h"
#include <algorithm>

typedef enum COM_VendorID { NVIDIA = 0x10DE, AMD = 0x1002 } COM_VendorID;
// const int DEFAULT_NVIDIA_GLOBAL_DIM = 64;
// const int DEFAULT_OTHER_GLOBAL_DIM = 1024;

OpenCLDevice::OpenCLDevice(OpenCLManager &man, OpenCLPlatform &platform, cl_device_id device_id)
    : ComputeDevice(),
      m_queue(nullptr),
      m_man(man),
      m_platform(platform),
      m_device_id(device_id),
      m_device_type(),
      m_vendor_id(0),
      m_max_group_dims(),
      m_compute_units(0),
      m_clock_freq(0),
      m_global_mem_size(0),
      m_max_img_w(0),
      m_max_img_h(0),
      m_supported_formats()
{
}

OpenCLDevice::~OpenCLDevice()
{
  /* clew.h extension wrangler seem to have forgotten to declare the macro clReleaseDevice, so
   * we directly access __clewReleaseDevice function pointer*/
  m_man.printIfError(CLEW_GET_FUN(__clewReleaseDevice)(m_device_id));

  m_man.printIfError(clReleaseCommandQueue(m_queue));
}

void OpenCLDevice::initialize()
{
  if (!isInitialized()) {
    cl_int cl_int_result;

    m_man.printIfError(
        clGetDeviceInfo(m_device_id, CL_DEVICE_VENDOR_ID, sizeof(cl_int), &m_vendor_id, NULL));

    m_man.printIfError(clGetDeviceInfo(
        m_device_id, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_int), &cl_int_result, NULL));

    m_compute_units = cl_int_result;

    m_man.printIfError(clGetDeviceInfo(m_device_id,
                                       CL_DEVICE_MAX_WORK_ITEM_SIZES,
                                       sizeof(m_max_group_dims),
                                       &m_max_group_dims,
                                       NULL));
    m_man.printIfError(clGetDeviceInfo(
        m_device_id, CL_DEVICE_IMAGE2D_MAX_WIDTH, sizeof(m_max_img_w), &m_max_img_w, NULL));
    m_man.printIfError(clGetDeviceInfo(
        m_device_id, CL_DEVICE_IMAGE2D_MAX_HEIGHT, sizeof(m_max_img_h), &m_max_img_h, NULL));

    cl_device_type device_type = 0;
    m_man.printIfError(
        clGetDeviceInfo(m_device_id, CL_DEVICE_TYPE, sizeof(cl_device_type), &device_type, NULL));
    switch (device_type) {
      case CL_DEVICE_TYPE_CPU:
        m_device_type = ComputeDeviceType::CPU;
        break;
      case CL_DEVICE_TYPE_GPU:
        m_device_type = ComputeDeviceType::GPU;
        break;
      default:
        BLI_assert(!"Unimplemented opencl device type");
        break;
    }

    cl_int error;
    m_queue = clCreateCommandQueue(m_platform.getContext(), m_device_id, 0, &error);
    m_man.printIfError(error);

    ComputeDevice::initialize();
  }
}

void OpenCLDevice::enqueueWork(int work_width,
                               int work_height,
                               std::string kernel_name,
                               std::function<void(ComputeKernel *)> add_kernel_args_func)
{
  ComputeKernelUniquePtr kernel_uptr = m_platform.takeKernel(kernel_name, this);
  auto kernel = (OpenCLKernel *)kernel_uptr.get();
  add_kernel_args_func(kernel);

  /* Kernel may have added argument buffers that need write work. We have to execute it now,
  otherwise arguments will be empty when calling clEnqueueNDRangeKernel */
  if (kernel->hasWorkEnqueued()) {
    waitQueueToFinish();
  }

  int max_global_w = m_max_group_dims[0] / 2;
  int max_global_h = m_max_group_dims[1] / 2;
  int width_step = max_global_w > work_width ? work_width : max_global_w;
  int height_step = max_global_h > work_height ? work_height : max_global_h;
  for (int work_y = 0; work_y < work_height; work_y += height_step) {
    for (int work_x = 0; work_x < work_width; work_x += width_step) {
      int x_size = (work_width - work_x) > width_step ? width_step : (work_width - work_x);
      int y_size = (work_height - work_y) > height_step ? height_step : (work_height - work_y);

      size_t global_work_offset[2] = {(size_t)work_x, (size_t)work_y};
      size_t global_work_size[2] = {(size_t)x_size, (size_t)y_size};
      m_man.printIfError(clEnqueueNDRangeKernel(m_queue,
                                                kernel->getClKernel(),
                                                2,
                                                global_work_offset,
                                                global_work_size,
                                                NULL,
                                                0,
                                                NULL,
                                                NULL));
      clFlush(m_queue);
    }
  }
}

void OpenCLDevice::waitQueueToFinish()
{
  clFinish(m_queue);
}

void *OpenCLDevice::allocImageBuffer(ComputeAccess mem_access,
                                     int width,
                                     int height,
                                     bool alloc_for_host_map)
{
  cl_int error;
  int mem_flags = m_platform.getMemoryAccessFlag(mem_access);
  if (alloc_for_host_map) {
    mem_flags |= CL_MEM_ALLOC_HOST_PTR;
  }
  cl_mem cl_img = clCreateImage2D(m_platform.getContext(),
                                  mem_flags,
                                  m_platform.getImageFormat(),
                                  width,
                                  height,
                                  0,
                                  NULL,
                                  &error);
  m_man.printIfError(error);
  return cl_img;
}

void *OpenCLDevice::allocGenericBuffer(ComputeAccess mem_access,
                                       size_t bytes,
                                       bool alloc_for_host_map)
{
  cl_int error;
  int mem_flags = m_platform.getMemoryAccessFlag(mem_access);
  if (alloc_for_host_map) {
    mem_flags |= CL_MEM_ALLOC_HOST_PTR;
  }
  cl_mem cl_img = clCreateBuffer(m_platform.getContext(), mem_flags, bytes, NULL, &error);
  m_man.printIfError(error);
  return cl_img;
}

float *OpenCLDevice::mapImageBufferToHostEnqueue(void *device_buffer,
                                                 ComputeAccess host_mem_access,
                                                 int width,
                                                 int height,
                                                 size_t &r_map_row_bytes)
{
  cl_int error;

  int map_flags = 0;
  if (host_mem_access == ComputeAccess::READ) {
    map_flags = CL_MAP_READ;
  }
  else if (host_mem_access == ComputeAccess::WRITE) {
    map_flags = CL_MAP_WRITE_INVALIDATE_REGION;
  }
  else {
    map_flags = CL_MAP_READ | CL_MAP_WRITE;
  }
  cl_mem img = (cl_mem)device_buffer;
  size_t origin[3] = {0, 0, 0};
  size_t size[3] = {(size_t)width, (size_t)height, 1};
  float *map = (float *)clEnqueueMapImage(m_queue,
                                          img,
                                          CL_FALSE,
                                          map_flags,
                                          origin,
                                          size,
                                          &r_map_row_bytes,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL,
                                          &error);
  m_man.printIfError(error);
  return map;
}

void OpenCLDevice::unmapBufferFromHostEnqueue(void *device_buffer, void *host_mapped_buffer)
{
  cl_mem img = (cl_mem)device_buffer;
  m_man.printIfError(clEnqueueUnmapMemObject(m_queue, img, host_mapped_buffer, 0, NULL, NULL));
}

void OpenCLDevice::freeBuffer(void *device_buffer)
{
  cl_mem img = (cl_mem)device_buffer;
  m_man.printIfError(clReleaseMemObject(img));
}