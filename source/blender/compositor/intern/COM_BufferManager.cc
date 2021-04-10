/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Copyright 2021, Blender Foundation.
 */

#include "COM_BufferManager.h"
#include "COM_ComputeDevice.h"
#include "COM_ComputeManager.h"

namespace blender::compositor {

static int getImageElemChs(DataType image_type)
{
  switch (image_type) {
    case DataType::Value:
      return 1;
    case DataType::Vector:
      return 3;
    case DataType::Color:
      return 4;
    default:
      BLI_assert(!"Non implemented DataType");
      return 0;
  }
}

static int getImageElemStride(DataType image_type, bool gpu_enabled)
{
  if (gpu_enabled) {
    /*
     * We use RGBA image format even for 3 (VECTOR) or 1 (VALUE) channels so that we can map all
     * image buffers between GPU and CPU reusing them independently of the number of channels. It's
     * much faster than copying/allocating.
     *
     * Furthermore OpenCL 1.1 only guarantees to support image formats CL_RGBA or CL_BGRA on all
     * devices. See:
     * https://www.khronos.org/registry/OpenCL/sdk/1.1/docs/man/xhtml/clGetSupportedImageFormats.html
     */
    return 4;
  }
  else {
    return getImageElemChs(image_type);
  }
}

static void setBaseBuffer(BaseBuffer *buf,
                          BufferType type,
                          int width,
                          int height,
                          int elem_chs,
                          int elem_stride,
                          int row_stride,
                          size_t ch_bytes,
                          bool is_single_elem,
                          size_t orig_total_bytes = 0)
{
  buf->ch_bytes = ch_bytes;
  buf->elem_bytes = elem_chs * ch_bytes;
  buf->elem_chs = elem_chs;
  buf->elem_stride_bytes = elem_stride * ch_bytes;
  buf->width = width;
  buf->height = height;
  buf->is_single_elem = is_single_elem;
  BLI_rcti_init(&buf->rect, 0, width, 0, height);
  buf->type = type;
  buf->row_stride_bytes = buf->getMemWidth() * buf->elem_stride_bytes;
  buf->used_bytes = buf->getMemHeight() * buf->row_stride_bytes;
  buf->total_bytes = orig_total_bytes == 0 ? buf->used_bytes : orig_total_bytes;
  if (is_single_elem) {
    buf->elem_jump = 0;
    buf->row_jump = 0;
    buf->row_pad_jump = 0;
  }
  else {
    buf->elem_jump = elem_stride;
    buf->row_jump = row_stride;
    buf->row_pad_jump = row_stride - (elem_stride * width);
  }
}

CPUBufferManager::CPUBufferManager(ComputeManager *compu_man)
    : m_gpu_buf_man(nullptr), m_compu_man(compu_man)
{
}

CPUBufferUniquePtr<float> CPUBufferManager::takeImageBuffer(DataType image_type,
                                                            int width,
                                                            int height,
                                                            bool is_single_elem)
{
  bool gpu_enabled = m_compu_man->canCompute();
  int elem_chs = getImageElemChs(image_type);
  if (gpu_enabled) {
    /* Try get a CPU buffer map from GPU to unmap it later on if GPU access is needed (much
     * faster than copying to GPU). When mapping between GPU and CPU: width, height and any
     * variable that may affect memory model must be kept. */
    BaseCPUBuffer *buf = takeRecycledBuffer(BufferType::IMAGE_MAP, width, height, is_single_elem);
    if (buf) {
      buf->elem_chs = elem_chs;
      return asCPUBuffer<float>(manageBuffer(buf));
    }
    else {
      /* No CPU buffer map from GPU found. Take a GPUBuffer from GPUBufferManager and adapt it
       * to CPU (map it)*/
      GPUBufferUniquePtr gpu_buf = m_gpu_buf_man->takeImageBuffer(
          image_type, width, height, is_single_elem);
      BaseBufferUniquePtr base_buf = m_gpu_buf_man->asBaseBuffer(std::move(gpu_buf));
      CPUBufferUniquePtr<float> cpu_buf = adaptImageBuffer(std::move(base_buf));
      cpu_buf->elem_chs = elem_chs;
      return cpu_buf;
    }
  }
  else {
    int elem_stride = getImageElemStride(image_type, gpu_enabled);
    size_t ch_bytes = sizeof(float);
    auto buf = takeMinSizeBuffer(BufferType::IMAGE,
                                 width,
                                 height,
                                 elem_chs,
                                 elem_stride,
                                 ch_bytes,
                                 is_single_elem,
                                 {BufferType::IMAGE, BufferType::GENERIC},
                                 [] { return new CPUBuffer<float>(); });
    return asCPUBuffer<float>(std::move(buf));
  }
}

CPUBufferUniquePtr<float> CPUBufferManager::adaptImageBuffer(BaseBufferUniquePtr buf)
{
  BLI_assert(buf->type == BufferType::IMAGE || buf->type == BufferType::IMAGE_MAP);
  if (typeid(*buf.get()) == typeid(GPUBuffer)) {
    GPUBuffer *gpu_buf = (GPUBuffer *)buf.get();

    /* map gpu buffer to cpu */
    BLI_assert(m_compu_man->canCompute());
    size_t map_row_bytes;
    float *cpu_map = m_compu_man->getSelectedDevice()->mapImageBufferToHostEnqueue(
        gpu_buf->buf,
        ComputeAccess::READ_WRITE,
        gpu_buf->getMemWidth(),
        gpu_buf->getMemHeight(),
        map_row_bytes);

    /* wrap cpu map and mapped gpu buf in a CPUBuffer */
    BaseCPUBuffer *cpu_buf = new CPUBuffer<float>();
    cpu_buf->buf = cpu_map;
    cpu_buf->mapped_gpu_buf = gpu_buf;
    buf.release();  // release gpu buffer ownership as it will be managed with CPUBuffer map

    int elem_stride = static_cast<int>(gpu_buf->elem_stride_bytes / gpu_buf->ch_bytes);
    int row_stride = static_cast<int>(map_row_bytes / gpu_buf->ch_bytes);
    setBaseBuffer(cpu_buf,
                  BufferType::IMAGE_MAP,
                  gpu_buf->width,
                  gpu_buf->height,
                  gpu_buf->elem_chs,
                  elem_stride,
                  row_stride,
                  gpu_buf->ch_bytes,
                  gpu_buf->is_single_elem);

    return asCPUBuffer<float>(manageBuffer(cpu_buf));
  }
  else {
    return asCPUBuffer<float>(asBaseCPUBuffer(std::move(buf)));
  }
}

void CPUBufferManager::setGPUBufferManager(GPUBufferManager *man)
{
  m_gpu_buf_man = man;
}

BaseCPUBufferUniquePtr CPUBufferManager::asBaseCPUBuffer(BaseBufferUniquePtr buf)
{
  /* Assert buf is a cpu buffer */
  BLI_assert(dynamic_cast<BaseCPUBuffer *>(buf.get()) != nullptr);
  return BaseCPUBufferUniquePtr((BaseCPUBuffer *)buf.release(), buf.get_deleter());
}

BaseCPUBufferUniquePtr CPUBufferManager::takeMinSizeBuffer(
    BufferType buf_type,
    int width,
    int height,
    int elem_chs,
    int elem_stride,
    size_t ch_bytes,
    bool is_single_elem,
    std::initializer_list<BufferType> recycle_types,
    std::function<BaseCPUBuffer *()> cpu_buf_allocator)
{
  size_t min_bytes = is_single_elem ? elem_stride * ch_bytes :
                                      (size_t)width * elem_stride * height * ch_bytes;
  BaseCPUBuffer *buf = nullptr;
  for (BufferType recycle_type : recycle_types) {
    buf = takeRecycledBuffer(recycle_type, min_bytes);
    if (buf) {
      break;
    }
  }

  size_t total_bytes = 0;
  if (buf) {
    total_bytes = buf->total_bytes;
  }
  else {
    buf = cpu_buf_allocator();
    total_bytes = min_bytes;
    buf->buf = allocRawBuffer(total_bytes);
  }

  int row_stride = is_single_elem ? elem_stride : elem_stride * width;
  setBaseBuffer(buf,
                buf_type,
                width,
                height,
                elem_chs,
                elem_stride,
                row_stride,
                ch_bytes,
                total_bytes,
                is_single_elem);

  return manageBuffer(buf);
}

void *CPUBufferManager::allocRawBuffer(size_t bytes)
{
  return MEM_mallocN_aligned(bytes, 16, "CPUBufferManager::allocRawBuffer");
}

void CPUBufferManager::freeBuffer(BaseCPUBuffer *cpu_buf)
{
  if (cpu_buf->type == BufferType::IMAGE_MAP) {
    m_compu_man->getSelectedDevice()->unmapBufferFromHostEnqueue(cpu_buf->mapped_gpu_buf->buf,
                                                                 cpu_buf->buf);
    m_compu_man->getSelectedDevice()->waitQueueToFinish();
    m_gpu_buf_man->freeBuffer(cpu_buf->mapped_gpu_buf);
  }
  else {
    MEM_freeN(cpu_buf->buf);
  }
  delete cpu_buf;
}

GPUBufferManager::GPUBufferManager(ComputeManager *compu_man)
    : m_cpu_buf_man(nullptr), m_compu_man(compu_man)
{
}

GPUBufferUniquePtr GPUBufferManager::takeImageBuffer(DataType image_type,
                                                     int width,
                                                     int height,
                                                     bool is_single_elem)
{
  int elem_chs = getImageElemChs(image_type);
  GPUBuffer *buf = takeRecycledBuffer(BufferType::IMAGE, width, height, is_single_elem);
  if (buf) {
    buf->elem_chs = elem_chs;
    return manageBuffer(buf);
  }
  else {
    /* No gpu buffer found. Try get a cpu buffer map and unmap it. */
    BaseCPUBuffer *cpu_buf = m_cpu_buf_man->takeRecycledBuffer(
        BufferType::IMAGE_MAP, width, height, is_single_elem);
    if (cpu_buf) {
      cpu_buf->elem_chs = elem_chs;
      auto base_buf = BaseBufferUniquePtr(cpu_buf, [](BaseBuffer *b) {});
      return adaptImageBuffer(std::move(base_buf));
    }
    else {
      /* No cpu buffer map found. Create new gpu buffer. */
      buf = new GPUBuffer();

      int elem_stride = getImageElemStride(image_type, true);
      size_t ch_bytes = sizeof(float);
      /* The internal row stride for gpu memory is unknown at this point so we just set the
       * default. It's only needed and known when mapping. */
      int row_stride = width * elem_stride;
      setBaseBuffer(buf,
                    BufferType::IMAGE,
                    width,
                    height,
                    elem_chs,
                    elem_stride,
                    row_stride,
                    ch_bytes,
                    is_single_elem);

      void *gpu_img_buf = allocImageBuffer(buf->getMemWidth(), buf->getMemHeight());
      buf->buf = gpu_img_buf;

      return manageBuffer(buf);
    }
  }

  return GPUBufferUniquePtr(nullptr, [](BaseBuffer *buf) {});
}

GPUBufferUniquePtr GPUBufferManager::adaptImageBuffer(BaseBufferUniquePtr buf)
{
  if (typeid(*buf.get()) == typeid(BaseCPUBuffer)) {
    BLI_assert(buf->type == BufferType::IMAGE_MAP);

    BaseCPUBuffer *cpu_buf = (BaseCPUBuffer *)buf.get();
    m_compu_man->getSelectedDevice()->unmapBufferFromHostEnqueue(cpu_buf->mapped_gpu_buf,
                                                                 cpu_buf->buf);
    buf.release();  // release cpu buffer ownership as we have unmapped it

    GPUBuffer *gpu_buf = cpu_buf->mapped_gpu_buf;
    return asGPUBuffer(manageBuffer(gpu_buf));
  }
  else {
    BLI_assert(typeid(*buf.get()) == typeid(GPUBuffer));
    BLI_assert(buf->type == BufferType::IMAGE);
    return asGPUBuffer(std::move(buf));
  }
}

void GPUBufferManager::setCPUBufferManager(CPUBufferManager *man)
{
  m_cpu_buf_man = man;
}

void GPUBufferManager::freeBuffer(GPUBuffer *gpu_buf)
{
  m_compu_man->getSelectedDevice()->freeBuffer(gpu_buf->buf);
  delete gpu_buf;
}

void *GPUBufferManager::allocImageBuffer(int width, int height)
{
  return m_compu_man->getSelectedDevice()->allocImageBuffer(
      ComputeAccess::READ_WRITE, width, height, true);
}
void *GPUBufferManager::allocGenericBuffer(size_t bytes)
{
  return m_compu_man->getSelectedDevice()->allocGenericBuffer(
      ComputeAccess::READ_WRITE, bytes, false);
}

GPUBufferUniquePtr GPUBufferManager::asGPUBuffer(BaseBufferUniquePtr buf)
{
  /* Assert buf is a gpu buffer */
  BLI_assert(dynamic_cast<GPUBuffer *>(buf.get()) != nullptr);
  return GPUBufferUniquePtr((GPUBuffer *)buf.release(), buf.get_deleter());
}

}  // namespace blender::compositor