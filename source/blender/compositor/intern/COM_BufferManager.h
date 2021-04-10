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

#pragma once

#include "COM_Buffer.h"

#include <set>
#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif

class ComputeManager;
namespace blender::compositor {

#define COM_RECYCLING_ENABLED 1
constexpr double COM_RECYCLING_FACTOR = 5.0;

template<typename TBuffer,
         typename std::enable_if<std::is_base_of<BaseBuffer, TBuffer>::value>::type * = nullptr>
class BufferManager {
  typedef std::unique_ptr<TBuffer, BufferManagerDeleter> TBufferUniquePtr;

 private:
  std::set<TBuffer *> m_recycled_bufs;

 public:
  BaseBufferUniquePtr asBaseBuffer(TBufferUniquePtr buf)
  {
    return BaseBufferUniquePtr(buf.release(), buf.get_deleter());
  }

  virtual ~BufferManager()
  {
    freeRecycledBuffers();
  }

 protected:
  virtual void freeBuffer(TBuffer *buf) = 0;

  void freeRecycledBuffers()
  {
    for (auto buf : m_recycled_bufs) {
      freeBuffer(buf);
    }
    m_recycled_bufs.clear();
  }

  TBufferUniquePtr manageBuffer(TBuffer *buf)
  {
#ifdef COM_RECYCLING_ENABLED
    auto deleter = [this](BaseBuffer *base_buf) { recycleBuffer((TBuffer *)base_buf); };
#else
    auto deleter = [this](BaseBuffer *base_buf) { freeBuffer((TBuffer *)base_buf); };
#endif
    return TBufferUniquePtr(buf, deleter);
  }

  TBuffer *takeRecycledBuffer(BufferType type, size_t min_bytes)
  {
    TBuffer *best = nullptr;
    for (auto buf : m_recycled_bufs) {
      if (buf->type == type && buf->total_bytes >= min_bytes &&
          buf->total_bytes <= COM_RECYCLING_FACTOR * min_bytes) {
        if (best == nullptr || buf->total_bytes < best->total_bytes) {
          best = buf;
        }
      }
    }
    return best;
  }

  TBuffer *takeRecycledBuffer(BufferType type, int width, int height, bool is_single_elem)
  {
    if (is_single_elem) {
      for (auto buf : m_recycled_bufs) {
        if (buf->type == type && buf->is_single_elem) {
          return buf;
        }
      }
    }
    else {
      for (auto buf : m_recycled_bufs) {
        if (buf->type == type && buf->width == width && buf->height == height) {
          return buf;
        }
      }
    }
    return nullptr;
  }

 private:
  void recycleBuffer(TBuffer *buf)
  {
    m_recycled_bufs.insert(buf);
  }

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:BufferManager")
#endif
};

class GPUBufferManager;
class CPUBufferManager : public BufferManager<BaseCPUBuffer> {
 private:
  GPUBufferManager *m_gpu_buf_man;
  ComputeManager *m_compu_man;

 public:
  CPUBufferManager(ComputeManager *compu_man);
  void setGPUBufferManager(GPUBufferManager *man);
  CPUBufferUniquePtr<float> takeImageBuffer(DataType img_type,
                                            int width,
                                            int height,
                                            bool is_single_elem = false);
  CPUBufferUniquePtr<float> adaptImageBuffer(BaseBufferUniquePtr buf);
  template<typename T>
  CPUBufferUniquePtr<T> takeGenericBuffer(int width, int height = 1, int elem_chs = 1)
  {
    size_t ch_bytes = sizeof(T);
    int elem_stride = elem_chs;
    auto buf = takeMinSizeBuffer(BufferType::GENERIC,
                                 width,
                                 height,
                                 elem_chs,
                                 elem_stride,
                                 ch_bytes,
                                 false,
                                 {BufferType::GENERIC, BufferType::IMAGE},
                                 []() { return new CPUBuffer<T>(); });
    return asCPUBuffer<T>(std::move(buf));
  }

  template<typename T> BaseBufferUniquePtr asBaseBuffer(CPUBufferUniquePtr<T> buf)
  {
    return BaseBufferUniquePtr(buf.release(), buf.get_deleter());
  }

  template<typename T> BaseCPUBufferUniquePtr asBaseCPUBuffer(CPUBufferUniquePtr<T> buf)
  {
    return BaseCPUBufferUniquePtr((BaseCPUBuffer *)buf.release(), buf.get_deleter());
  }

 protected:
  void freeBuffer(BaseCPUBuffer *buf) override;

 private:
  BaseCPUBufferUniquePtr asBaseCPUBuffer(BaseBufferUniquePtr buf);

  template<typename T> CPUBufferUniquePtr<T> asCPUBuffer(BaseCPUBufferUniquePtr buf)
  {
    /* Assert base cpu buffer is of the given type */
    BLI_assert(dynamic_cast<CPUBuffer<T> *>(buf.get()) != nullptr);
    auto cpu_buf = CPUBufferUniquePtr<T>((CPUBuffer<T> *)buf.release(), buf.get_deleter());
    cpu_buf->typed_buf = (T *)cpu_buf->buf;
    return cpu_buf;
  }

  BaseCPUBufferUniquePtr takeMinSizeBuffer(BufferType buf_type,
                                           int width,
                                           int height,
                                           int elem_chs,
                                           int elem_stride,
                                           size_t ch_bytes,
                                           bool is_single_elem,
                                           std::initializer_list<BufferType> recycle_types,
                                           std::function<BaseCPUBuffer *()> cpu_buf_allocator);
  void *allocRawBuffer(size_t bytes);

  friend class GPUBufferManager;

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:CPUBufferManager");
#endif
};

class GPUBufferManager : public BufferManager<GPUBuffer> {
 private:
  CPUBufferManager *m_cpu_buf_man;
  ComputeManager *m_compu_man;

 public:
  GPUBufferManager(ComputeManager *compu_man);
  void setCPUBufferManager(CPUBufferManager *man);
  GPUBufferUniquePtr takeImageBuffer(DataType img_type,
                                     int width,
                                     int height,
                                     bool is_single_elem = false);
  GPUBufferUniquePtr adaptImageBuffer(BaseBufferUniquePtr buf);

 protected:
  void freeBuffer(GPUBuffer *buf) override;

 private:
  void *allocImageBuffer(int width, int height);
  void *allocGenericBuffer(size_t bytes);
  GPUBufferUniquePtr asGPUBuffer(BaseBufferUniquePtr buf);

  friend class CPUBufferManager;

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:GPUBufferManager");
#endif
};

}  // namespace blender::compositor
