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

#ifndef __COM_BUFFERMANAGER_H__
#define __COM_BUFFERMANAGER_H__

#include "COM_ReadsOptimizer.h"
#include "COM_Rect.h"
#include "DNA_vec_types.h"
#include "MEM_guardedalloc.h"
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

class NodeOperation;
class ExecutionSystem;
class ReadsOptimizer;
struct CacheBuffer;
class ExecutionManager;
class BufferRecycler;
class CompositorContext;
struct OpReads;

class BufferManager {
 public:
  typedef struct ReadResult {
    /* if not written, pixels will be empty. You must call writeSeek first and then call back
     * readSeek*/
    bool is_written;
    /* if written, pixels for the given op_rect will be returned */
    std::shared_ptr<PixelsRect> pixels;
  } ReadResult;

 private:
  bool m_initialized;
  std::unordered_map<OpKey, ReadsOptimizer *> m_optimizers;
  std::unordered_map<OpKey, CacheBuffer *> m_cached_buffers;
  std::unique_ptr<BufferRecycler> m_recycler;
  std::unordered_map<OpKey, std::vector<ReaderReads *>> m_readers_reads;
  bool m_readers_reads_gotten;
  size_t m_max_cache_bytes;
  size_t m_current_cache_bytes;

 public:
  BufferManager();
  bool isInitialized()
  {
    return m_initialized;
  }
  void initialize(const CompositorContext &context);
  void deinitialize(bool isBreaked);

  void readOptimize(NodeOperation *op, NodeOperation *reader_op, ExecutionManager &man);
  /* returns as first param whether it is written and could be read. And second the read rect.
  If couldn't be read, writeSeek must be called first and then call back readSeek*/
  BufferManager::ReadResult readSeek(NodeOperation *op,
                                     NodeOperation *reader_op,
                                     ExecutionManager &man);
  void writeSeek(NodeOperation *op,
                 ExecutionManager &man,
                 std::function<void(TmpRectBuilder &)> write_func);
  bool hasBufferCache(NodeOperation *op);

  ~BufferManager();

 private:
  void checkCache();
  CacheBuffer *getCache(NodeOperation *op);
  TmpBuffer *getCustomBuffer(NodeOperation *op);
  bool prepareForWrite(bool is_write_computed, OpReads *reads);
  bool prepareForRead(bool is_compute_written, OpReads *reads);
  void reportWriteCompleted(NodeOperation *op, ExecutionManager &man);

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:BufferManager")
#endif
};

#endif
