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

#include "COM_BaseCache.h"
#include "COM_BaseConverter.h"
#include <mutex>
#include <thread>
#include <unordered_set>

class DiskCache : public BaseCache {
 private:
  const char *CACHE_INNER_DIR_NAME = "blender_cmpcache";
  std::string m_cache_dir_path;
  std::thread m_load_thread;

  OpKey m_prefetch_key;
  float *m_prefetched_cache;
  std::thread m_prefetch_thread;

  std::unordered_map<OpKey, std::thread> m_save_threads;
  std::unordered_set<OpKey> m_save_ended_threads;
  std::unordered_map<OpKey, std::thread> m_delete_threads;
  std::unordered_set<OpKey> m_delete_ended_threads;

  std::mutex m_save_ended_mutex;
  std::mutex m_delete_ended_mutex;

  bool m_cache_dir_set;

  BaseConverter m_base_convert;

 public:
  DiskCache(size_t op_type_hash);
  virtual ~DiskCache() override;

  virtual void initialize(const CompositorContext *ctx) override;
  virtual void deinitialize(const CompositorContext *ctx) override;
  virtual void deleteAllCaches() override;

 protected:
  virtual void saveCache(const CacheInfo *info,
                         float *data,
                         std::function<void()> on_save_end) override;
  virtual void prefetchCache(const CacheInfo *info) override;
  // returns null if there were any problem retrieving the cache (e.g. deleted by user on disk)
  virtual float *getCache(const CacheInfo *info) override;
  virtual void deleteCache(const CacheInfo *info) override;

  // Removes cache entry but doesn't delete the cached buffer. The cached buffer is returned
  // instead. Only for memory based caches, other types will throw exception when called.
  virtual float *removeCache(const CacheInfo *info) override;

  virtual size_t getMaxBytes(const CompositorContext *ctx) override;
  bool doesGetCacheReturnsCopy() override
  {
    return true;
  }

 private:
  // assures that if previous prefetched cache has not been gotten (when gotten is set to nullptr),
  // it's freed.
  void assurePreviousPrefetchFreed();
  void removeEndedThreads();
  void loadCacheDir();
  void joinRelatedThreads(const CacheInfo *info);
  void joinAllThreads();
  std::string getFilePath(const CacheInfo *cache_info);
  // returns whether its a valid filename and the op key with last_use_time if true.
  std::tuple<bool, OpKey, uint64_t> getCacheInfoFromFilename(const std::string filename);

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:DiskCache")
#endif
};
