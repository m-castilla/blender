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

#include <memory>
#include <unordered_map>

#include "COM_CacheOperation.h"
#include "COM_DiskCache.h"
#include "COM_Keys.h"
#include "COM_MemoryCache.h"
#include "COM_ViewCacheManager.h"

class BufferRecycler;
class PixelsRect;
class CompositorContext;
class CacheManager {
 private:
  std::unique_ptr<BaseCache> m_disk_cache;
  std::unique_ptr<BaseCache> m_mem_cache;
  ViewCacheManager m_view_cache_man;
  std::unordered_map<PersistentKey, OpKey> m_persistent_map;
  const CompositorContext *m_ctx;
  BufferRecycler *m_recycler;
  std::pair<PersistentKey, OpKey> m_last_get_cache_persist;

 public:
  CacheManager();
  ~CacheManager();

  void setOperationMode(OperationMode mode);

  void initialize(const CompositorContext *ctx, BufferRecycler *recycler);
  void deinitialize(const CompositorContext *ctx);

  ViewCacheManager *getViewCacheMan()
  {
    return &m_view_cache_man;
  }

  TmpBuffer *getCachedOrNewAndPrefetchNext(NodeOperation *op);
  // returns nullptr if not found or there was an error getting the cache
  TmpBuffer *getCache(NodeOperation *op, bool prefetch_next = false);
  void cacheReadOptimize(NodeOperation *op);
  bool hasCache(NodeOperation *op);
  bool isCacheable(NodeOperation *op);
  bool isCacheableAndPersistent(NodeOperation *op);
  std::pair<bool, OpKey> checkPersistentOpKey(NodeOperation *op);

  // Checks for normal cache with CacheOperation and view cache. For either of the two will return
  // true
  bool hasAnyKindOfCache(NodeOperation *op);

 private:
  bool hasCache(const OpKey &key);
  PersistentKey buildPersistentKey(NodeOperation *op);
  void checkCaches();

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:CacheManager")
#endif
};
