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

class MemoryCache : public BaseCache {
 private:
  std::unordered_map<OpKey, float *> m_caches;

 public:
  MemoryCache(size_t op_type_hash);
  virtual ~MemoryCache() override;
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
  virtual float *removeCache(const CacheInfo *info) override;

  virtual size_t getMaxBytes(const CompositorContext *ctx) override;
  bool doesGetCacheReturnsCopy() override
  {
    return false;
  }

 private:
#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:MemoryCache")
#endif
};
