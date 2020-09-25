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

#include <deque>
#include <memory>
#include <set>
#include <stddef.h>
#include <string>
#include <unordered_map>
#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif

#include "COM_Keys.h"

typedef struct RemovedCache {
  // might be null when cache couldn't be retrieved (e.g. user deleted it from disk)
  float *data;
  OpKey op_key;
  uint64_t last_use_time;
  ~RemovedCache();

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:RemovedCache")
#endif
} RemovedCache;

class CompositorContext;
class CacheOperation;
class BaseCache {
 public:
  BaseCache(size_t op_type_hash);
  // Must be overriden by base classes to delete all caches on destroy.
  virtual ~BaseCache();

 protected:
  typedef struct CacheInfo {
    OpKey op_key;
    // in nanoseoncds from linux epoch
    uint64_t last_use_time;

    size_t getTotalBytes() const;
#ifdef WITH_CXX_GUARDEDALLOC
    MEM_CXX_CLASS_ALLOC_FUNCS("COM:CacheInfo")
#endif
  } CacheInfo;

 private:
  struct SortBySaveTime {
    bool operator()(const CacheInfo *lhs, const CacheInfo *rhs) const
    {
      return lhs->last_use_time < rhs->last_use_time;
    }
  };
  size_t m_op_type_hash;
  size_t m_max_bytes;
  size_t m_current_bytes;

  std::unordered_map<OpKey, CacheInfo *> m_caches;
  std::set<CacheInfo *, SortBySaveTime> m_caches_by_time;

  // Optimization calls order, which should be
  // the same as when in execution mode. Used for prefetching caches
  std::deque<OpKey> m_prefetch_queue;

  OperationMode m_op_mode;

 private:
  void deleteCacheInfo(CacheInfo *info);
  void prefetchNextCache();

 public:
  virtual void initialize(const CompositorContext *ctx);
  virtual void deinitialize(const CompositorContext *ctx);
  void setOperationMode(OperationMode mode);

  // Must be called to check total cache size fits within limits. Return caches that has been
  // removed when delete_caches argument is set to false, when the returned data is detroyed cache
  // data will be deleted too. When true, caches will be inmediately deleted and will always return
  // empty. delete_caches=false should only be set for memory based caches, other types of caches
  // will throw exception.
  std::vector<std::shared_ptr<RemovedCache>> checkCaches(bool delete_caches = true);

  void cacheReadOptimize(const OpKey &op_key);

  // last_use_time must only be given when the data being saved has been retrieved from other type
  // of cache
  void saveCache(const OpKey &op_key,
                 float *data,
                 std::function<void()> on_save_end = {},
                 uint64_t last_use_time = 0);

  // Returns null if there were any problem getting the cache (e.g. user deleted it from disk)
  float *getCache(const OpKey &op_key);
  // should be called only once per each cache during current execution. Returns null if there were
  // any problem getting the cache (e.g. user deleted it from disk)
  float *getCacheAndPrefetchNext(const OpKey &op_key);
  bool hasCache(const OpKey &op_key);

  // Must be overriden to delete specific type of cache resources. Allways call Base class method.
  virtual void deleteAllCaches();

  // Returns whether the getCache method returns a copy of the cache or the cache buffer itself.
  // When it is a copy it should be deleted after use. When it is not a copy, it should never be
  // modified or deleted, only read.
  virtual bool doesGetCacheReturnsCopy() = 0;

 protected:
  // op type is always be the same
  size_t getCacheOpTypeHash() const
  {
    return m_op_type_hash;
  }
  // may be called by subclasses on app initialization to load persistent caches infos.
  // last_use_time must be in nanoseoncds from linux epoch, if 0 current time will be set
  CacheInfo *loadCacheInfo(const OpKey &key, uint64_t last_use_time);

  // last_use_time must only be given when the data being saved has been retrieved from other type
  // of cache
  virtual void saveCache(const CacheInfo *info,
                         float *data,
                         std::function<void()> on_save_end) = 0;
  virtual void prefetchCache(const CacheInfo *info) = 0;
  // returns null if there were any problem retrieving the cache (e.g. deleted by user on disk)
  virtual float *getCache(const CacheInfo *info) = 0;
  virtual void deleteCache(const CacheInfo *info) = 0;
  virtual float *removeCache(const CacheInfo *info) = 0;
  virtual size_t getMaxBytes(const CompositorContext *ctx) = 0;

 private:
  CacheInfo *getCacheInfo(const OpKey &op_key);

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:BaseCache")
#endif
};
