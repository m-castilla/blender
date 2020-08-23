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
 * Copyright 2020, Blender Foundation.
 */

#ifndef __COM_BUFFERRECYCLER_H__
#define __COM_BUFFERRECYCLER_H__
#include "COM_Buffer.h"
#include "MEM_guardedalloc.h"
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>

class ExecutionManager;
enum class BufferRecycleType { DEVICE_CLEAR, DEVICE_HOST_ALLOC, DEVICE_HOST_MAPPED, HOST_CLEAR };
void ForeachBufferRecycleType(std::function<void(BufferRecycleType)> func);

class BufferRecycler {
 private:
  typedef struct RecycleData {
    std::unordered_set<TmpBuffer *> buffers;
  } RecycleData;
  std::unordered_map<BufferRecycleType, RecycleData *> m_recycle;
  std::unordered_set<TmpBuffer *> m_created_buffers;
  std::string m_execution_id;

 public:
  BufferRecycler();
  ~BufferRecycler();

  void setExecutionId(const std::string &execution_id);

  TmpBuffer *createTmpBuffer(bool is_host_recyclable = true);
  TmpBuffer *createTmpBuffer(
      bool is_host_recyclable, float *host_buffer, int width, int height, int n_channels);

  /* returns whether there has been work enqueued to device*/
  bool takeRecycle(BufferRecycleType type, TmpBuffer *dst, int width, int height, int elem_chs);
  void giveRecycle(TmpBuffer *src);

  void recycleCurrentExecNonRecycledBuffers();
  void deleteAllBuffers();
  void deleteNonRecycledBuffers();
  void assertRecycledEqualsCreatedBuffers();

 private:
  bool isCreatedBufferRecycled(TmpBuffer *created_buf);
  void deleteBuffers(bool deleteRecycledBuffers);
  void addRecycle(BufferRecycleType type, TmpBuffer *buf);
  /* returns whether it could find a reusable buffer and set it to dst or not */
  bool recycleFindAndSet(
      BufferRecycleType type, TmpBuffer *dst, int width, int height, int elem_chs);

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:BufferRecycler")
#endif
};

#endif
