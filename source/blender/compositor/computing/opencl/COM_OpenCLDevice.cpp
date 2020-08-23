#include "COM_OpenCLDevice.h"
#include "BLI_assert.h"
#include "COM_BufferUtil.h"
#include "COM_OpenCLKernel.h"
#include "COM_OpenCLManager.h"
#include "COM_OpenCLPlatform.h"
#include "COM_Pixels.h"
#include "COM_PixelsUtil.h"
#include "COM_Rect.h"
#include "COM_RectUtil.h"
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
      m_device_type(ComputeDeviceType::CPU),
      m_vendor_id(0),
      m_max_group_dims(),
      m_compute_units(0),
      m_clock_freq(0),
      m_global_mem_size(0),
      m_max_img_w(0),
      m_max_img_h(0),
      m_supported_formats(),
      m_one_elem_img(nullptr)
{
}

OpenCLDevice::~OpenCLDevice()
{
  /* clew.h extension wrangler seem to have forgotten to declare the macro clReleaseDevice, so
   * we directly access __clewReleaseDevice function pointer*/
  m_man.printIfError(CLEW_GET_FUN(__clewReleaseDevice)(m_device_id));

  m_man.printIfError(clReleaseCommandQueue(m_queue));
  if (m_one_elem_img) {
    memDeviceFree(m_one_elem_img);
  }
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

void OpenCLDevice::queueJob(PixelsRect &dst,
                            std::string kernel_name,
                            std::function<void(ComputeKernel *)> add_kernel_args_func)
{
  OpenCLKernel *kernel = (OpenCLKernel *)m_platform.getKernel(kernel_name, this);

  auto setOffsetArgsFunc = kernel->addWriteImgArgs(dst);
  add_kernel_args_func((ComputeKernel *)kernel);

  size_t img_width = (size_t)dst.getWidth();
  size_t img_height = (size_t)dst.getHeight();

#if 0
  size_t img_size = img_width * img_height;
  int group_size_multiple = kernel->getGroupSizeMultiple();
  int group_max_size = kernel->getMaxGroupSize();

  int max_img_n_fast_groups = img_size / group_size_multiple;
  int desired_n_fast_groups = std::min(max_img_n_fast_groups, m_compute_units * 4);
  int desired_fast_group_size = desired_n_fast_groups * group_size_multiple;
  if (desired_fast_group_size > group_max_size && group_size_multiple <= group_max_size) {
    int diff = desired_fast_group_size - group_max_size;
    int deincrement = (int)ceil(diff / (float)group_size_multiple);
    desired_fast_group_size -= group_size_multiple * deincrement;
  }

  auto splits = RectUtil::splitRect(img_width,
                                    img_height,
                                    desired_fast_group_size,
                                    group_max_size,
                                    m_max_group_dims[0],
                                    m_max_group_dims[1]);
  enqueueSplit(m_queue, splits.desired_split, 0, 0, kernel);
  enqueueSplit(m_queue,
               splits.right_split,
               splits.desired_split.row_length * splits.desired_split.rects_w,
               0,
               kernel);
  enqueueSplit(m_queue,
               splits.top_split,
               0,
               splits.desired_split.col_length * splits.desired_split.rects_h,
               kernel);
#else
  int max_global_w = m_max_group_dims[0] / 2;
  int max_global_h = m_max_group_dims[1] / 2;
  int width_step = max_global_w > img_width ? img_width : max_global_w;
  int height_step = max_global_h > img_height ? img_height : max_global_h;
  for (int work_y = 0; work_y < img_height; work_y += height_step) {
    for (int work_x = 0; work_x < img_width; work_x += width_step) {
      int x_size = (img_width - work_x) > width_step ? width_step : (img_width - work_x);
      int y_size = (img_height - work_y) > height_step ? height_step : (img_height - work_y);

      setOffsetArgsFunc(work_x, work_y);
      size_t global_work_size[2] = {(size_t)x_size, (size_t)y_size};
      m_man.printIfError(clEnqueueNDRangeKernel(
          m_queue, kernel->getClKernel(), 2, NULL, global_work_size, NULL, 0, NULL, NULL));
      clFlush(m_queue);
    }
  }

#endif

  m_platform.recycleKernel((ComputeKernel *)kernel);
}

void OpenCLDevice::waitQueueToFinish()
{
  clFinish(m_queue);
}

void *OpenCLDevice::memDeviceAlloc(
    MemoryAccess mem_access, int width, int height, int elem_chs, bool alloc_for_host_map)
{
  cl_int error;
  int mem_flags = m_platform.getMemoryAccessFlag(mem_access);
  if (alloc_for_host_map) {
    mem_flags |= CL_MEM_ALLOC_HOST_PTR;
  }
  cl_mem cl_img = clCreateImage2D(m_platform.getContext(),
                                  mem_flags,
                                  m_platform.getImageFormat(elem_chs),
                                  width,
                                  height,
                                  0,
                                  NULL,
                                  &error);
  m_man.printIfError(error);
  return cl_img;
}

float *OpenCLDevice::memDeviceToHostMapEnqueue(void *device_buffer,
                                               MemoryAccess host_mem_access,
                                               int width,
                                               int height,
                                               int elem_chs,
                                               size_t &r_map_row_pitch)
{
  cl_int error;

  int map_flags = 0;
  if (host_mem_access == MemoryAccess::READ) {
    map_flags = CL_MAP_READ;
  }
  else if (host_mem_access == MemoryAccess::WRITE) {
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
                                          &r_map_row_pitch,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL,
                                          &error);
  BLI_assert(r_map_row_pitch >= BufferUtil::calcBufferRowBytes(width, elem_chs));
  m_man.printIfError(error);
  return map;
}

void OpenCLDevice::memDeviceToHostUnmapEnqueue(void *device_buffer, float *host_mapped_buffer)
{
  cl_mem img = (cl_mem)device_buffer;
  m_man.printIfError(
      clEnqueueUnmapMemObject(m_queue, img, (void *)host_mapped_buffer, 0, NULL, NULL));
}

void OpenCLDevice::memDeviceToHostCopyEnqueue(float *r_host_buffer,
                                              void *device_buffer,

                                              size_t host_row_bytes,
                                              MemoryAccess mem_access,
                                              int width,
                                              int height,
                                              int elem_chs)
{
  cl_mem img = (cl_mem)device_buffer;
  size_t origin[3] = {0, 0, 0};
  size_t size[3] = {(size_t)width, (size_t)height, 1};
  m_man.printIfError(clEnqueueReadImage(
      m_queue, img, CL_FALSE, origin, size, host_row_bytes, 0, r_host_buffer, 0, NULL, NULL));
}

void OpenCLDevice::memDeviceFree(void *device_buffer)
{
  cl_mem img = (cl_mem)device_buffer;
  m_man.printIfError(clReleaseMemObject(img));
}

void OpenCLDevice::memHostToDeviceCopyEnqueue(void *r_device_buffer,
                                              float *host_buffer,
                                              size_t host_row_bytes,
                                              MemoryAccess mem_access,
                                              int width,
                                              int height,
                                              int elem_chs)
{
  cl_mem img = (cl_mem)r_device_buffer;
  size_t origin[3] = {0, 0, 0};
  size_t size[3] = {(size_t)width, (size_t)height, 1};
  m_man.printIfError(clEnqueueWriteImage(
      m_queue, img, CL_FALSE, origin, size, host_row_bytes, 0, host_buffer, 0, NULL, NULL));
}

void OpenCLDevice::enqueueSplit(cl_command_queue cl_queue,
                                RectUtil::RectSplit split,
                                size_t offset_x,
                                size_t offset_y,
                                OpenCLKernel *kernel)
{
  if (split.n_rects > 0) {
    size_t equal_row_length = split.has_w_equal_rects ? split.row_length : split.row_length - 1;
    size_t equal_col_length = split.has_h_equal_rects ? split.col_length : split.col_length - 1;

    enqueueWork(cl_queue,
                offset_x,
                offset_y,
                equal_row_length,
                equal_col_length,
                split.rects_w,
                split.rects_h,
                kernel);
    if (!split.has_w_equal_rects) {
      enqueueWork(cl_queue,
                  offset_x + equal_row_length * split.rects_w,
                  offset_y,
                  1,
                  equal_col_length,
                  split.last_rect_in_row_w,
                  split.rects_h,
                  kernel);
    }
    if (!split.has_h_equal_rects) {
      enqueueWork(cl_queue,
                  offset_x,
                  offset_y + equal_col_length * split.rects_h,
                  equal_row_length,
                  1,
                  split.rects_w,
                  split.last_rect_in_col_h,
                  kernel);
    }
    if (!split.has_w_equal_rects && !split.has_h_equal_rects) {
      enqueueWork(cl_queue,
                  offset_x + equal_row_length * split.rects_w,
                  offset_y + equal_col_length * split.rects_h,
                  1,
                  1,
                  split.last_rect_in_row_w,
                  split.last_rect_in_col_h,
                  kernel);
    }
  }
}

void OpenCLDevice::enqueueWork(cl_command_queue cl_queue,
                               size_t offset_x,
                               size_t offset_y,
                               size_t row_length,
                               size_t col_length,
                               size_t rect_w,
                               size_t rect_h,
                               OpenCLKernel *kernel)
{
  if (row_length > 0 && col_length > 0 && rect_w > 0 && rect_h > 0) {
    size_t global_work_offset[2] = {offset_x, offset_y};
    size_t global_work_size[2] = {950, 950};
    size_t local_work_size[2] = {rect_w, rect_h};
    m_man.printIfError(clEnqueueNDRangeKernel(
        cl_queue, kernel->getClKernel(), 2, NULL, global_work_size, NULL, 0, NULL, NULL));
    clFlush(cl_queue);
  }
}

cl_mem OpenCLDevice::getOneElemImg()
{
  if (!m_one_elem_img) {
    m_one_elem_img = (cl_mem)memDeviceAlloc(MemoryAccess::READ, 1, 1, 1, false);
  }
  return m_one_elem_img;
}
