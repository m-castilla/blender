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
 * Copyright 2011, Blender Foundation.
 */

#pragma once

#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif
#include "BLI_math.h"
#include "BLI_rect.h"
#include "COM_defines.h"
#include <functional>
#include <memory>

namespace blender::compositor {
typedef enum MemoryBufferExtend {
  COM_MB_CLIP,
  COM_MB_EXTEND,
  COM_MB_REPEAT,
} MemoryBufferExtend;

enum class BufferType {
  /**
   * \brief CPU/GPU image buffer type.
   * GPU image buffers may be have a special memory model that may include element padding and row
   * padding. Width and height cannot be changed when recycling/mapping/unmapping GPU buffers.
   */
  IMAGE,
  /**
   * \brief CPU image buffer type that was mapped from GPU.
   * GPU image buffers may be have a special memory model that may include element and row
   * padding. Width and height cannot be changed when recycling/mapping/unmapping GPU buffers.
   */
  IMAGE_MAP,
  /**
   * \brief CPU/GPU buffer for general use.
   * They cannot be mapped between gpu and cpu.
   */
  GENERIC
};

struct BaseBuffer {
  /**
   * \brief Buffer type.
   * \see BufferType
   */
  BufferType type;

  /**
   * \brief Memory bytes available for use.
   * This is the size that was requested to buffer manager. It may be less than total_bytes when
   * recycling buffers.
   */
  size_t used_bytes;

  /**
   * \brief Buffer total memory bytes.
   */
  size_t total_bytes;

  /**
   * \brief Bytes size of a channel (sizeof(type)).
   */
  size_t ch_bytes;

  /**
   * \brief Number of channels in an element (or pixel).
   */
  int elem_chs;
  /**
   * \brief Bytes size of an element (without padding).
   * Elements may have padding when not all channels in an element are used (e.g. image buffer with
   * RGBA memory format (DataType::Color) but only R channel (DataType::Value) is used)
   */
  size_t elem_bytes;
  /**
   * \brief Bytes size of an element (with padding).
   * Elements may have padding when not all channels in an element are used (e.g. image buffer with
   * RGBA memory format (DataType::COLOR) but only R channel (DataType::VALUE) is used)
   */
  size_t elem_stride_bytes;
  /**
   * \brief Offset between elements.
   * When calculating buffer offsets this field should always be used for the x dimension.
   * e.g.: elem = &buffer[y * buffer.row_jump + x * buffer.elem_jump].
   * It will be 0 when is_single_elem=true.
   */
  int elem_jump;

  /**
   * \brief Buffer resolution width
   * Shouldn't be used for calculating buffer memory size. Single elem buffers may have any
   * resolution but it still is one element in memory.
   */
  int width;
  /**
   * \brief Buffer resolution height
   * Shouldn't be used for calculating buffer memory size. Single elem buffers may have any
   * resolution but it still is one element in memory.
   */
  int height;

  /**
   * \brief Bytes size of a row (including any padding).
   */
  size_t row_stride_bytes;
  /**
   * \brief Offset between the end of the last element of a row and the next row.
   * Depending on the buffer memory model, rows may have padding at the end. This could happen when
   * mapping gpu buffers to cpu.
   * It will be 0 when is_single_elem=true.
   */
  int row_pad_jump;
  /**
   * \brief Offset between rows.
   * When calculating buffer offsets this field should always be used for the y dimension.
   * e.g.: elem = &buffer[y * buffer.row_jump + x * buffer.elem_jump].
   * It will be 0 when is_single_elem=true.
   */
  int row_jump;

  /**
   * \brief Resolution rect
   * i.e. {0, width, 0, height}
   */
  rcti rect;

  /**
   * \brief Whether the buffer is a single element
   * A buffer with a single element in memory independently of its resolution (the element will be
   * repeated for all elements in the resolution). Used for set operations. This is needed to
   * reduce memory usage.
   */
  bool is_single_elem;

  /**
   * \brief Get number of elements in memory for a row. For single element buffers it will always
   * be 1.
   */
  int getMemWidth() const
  {
    return is_single_elem ? 1 : width;
  }
  /**
   * \brief Get number of elements in memory for a column. For single element buffers it will
   * always be 1.
   */
  int getMemHeight() const
  {
    return is_single_elem ? 1 : height;
  }

  int getOffset(int x, int y) const
  {
    return y * row_jump + x * elem_jump;
  }

  virtual ~BaseBuffer(){};

 protected:
  BaseBuffer()
  {
  }

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:BaseBuffer")
#endif
};

struct GPUBuffer : BaseBuffer {
  void *buf;

  friend class CPUBufferManager;
  friend class GPUBufferManager;

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:GpuBuffer")
#endif
};

struct BaseCPUBuffer : BaseBuffer {
 private:
  void *buf;
  GPUBuffer *mapped_gpu_buf;

  friend class CPUBufferManager;
  friend class GPUBufferManager;

 public:
  virtual ~BaseCPUBuffer(){};

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:BaseCPUBuffer")
#endif
};

template<typename T> struct CPUBuffer : BaseCPUBuffer {
 private:
  T *typed_buf;

  friend class CPUBufferManager;

 public:
  T &operator[](int index)
  {
    return typed_buf[index];
  }

  const T &operator[](int index) const
  {
    return typed_buf[index];
  }

  T *getElem(int x, int y)
  {
    return &typed_buf[getOffset(x, y)];
  }

  const T *getElem(int x, int y) const
  {
    return &typed_buf[getOffset(x, y)];
  }

  T &getValue(int x, int y, int ch)
  {
    return typed_buf[getOffset(x, y) + ch];
  }

  const T &getValue(int x, int y, int ch) const
  {
    return typed_buf[getOffset(x, y) + ch];
  }

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:CPUBuffer")
#endif
};

using BufferManagerDeleter = std::function<void(BaseBuffer *)>;
using BaseBufferUniquePtr = std::unique_ptr<BaseBuffer, BufferManagerDeleter>;
using BaseCPUBufferUniquePtr = std::unique_ptr<BaseCPUBuffer, BufferManagerDeleter>;
template<typename T>
using CPUBufferUniquePtr = std::unique_ptr<CPUBuffer<T>, BufferManagerDeleter>;
using GPUBufferUniquePtr = std::unique_ptr<GPUBuffer, BufferManagerDeleter>;
using MemoryBuffer = CPUBuffer<float>;

}  // namespace blender::compositor
