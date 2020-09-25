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

#include "COM_MemoryCache.h"
#include "COM_BufferUtil.h"
#include "COM_CompositorContext.h"

MemoryCache::MemoryCache(size_t op_type_hash) : BaseCache(op_type_hash)
{
}

MemoryCache::~MemoryCache()
{
  deleteAllCaches();
}

void MemoryCache::initialize(const CompositorContext *ctx)
{
  BaseCache::initialize(ctx);
}

void MemoryCache::deinitialize(const CompositorContext *ctx)
{
  BaseCache::deinitialize(ctx);
}

void MemoryCache::deleteAllCaches()
{
  BaseCache::deleteAllCaches();
  for (auto &it : m_caches) {
    if (it.second) {
      BufferUtil::hostFree(it.second);
    }
  }
  m_caches.clear();
}

void MemoryCache::saveCache(const CacheInfo *info, float *data, std::function<void()> on_save_end)
{
  auto find_it = m_caches.find(info->op_key);
  if (find_it != m_caches.end()) {
    float *old_cache = find_it->second;
    if (old_cache != nullptr) {
      BufferUtil::hostFree(old_cache);
    }
    find_it->second = data;
  }
  else {
    m_caches.emplace(info->op_key, data);
  }
  if (on_save_end) {
    on_save_end();
  }
}

void MemoryCache::prefetchCache(const CacheInfo *info)
{
  // for memory we don't need to prefetch
}

float *MemoryCache::getCache(const CacheInfo *info)
{
  auto find_it = m_caches.find(info->op_key);
  if (find_it != m_caches.end()) {
    return find_it->second;
  }
  else {
    return nullptr;
  }
}

void MemoryCache::deleteCache(const CacheInfo *info)
{
  float *data = getCache(info);
  if (data != nullptr) {
    BufferUtil::hostFree(data);
  }
  m_caches.erase(info->op_key);
}

float *MemoryCache::removeCache(const CacheInfo *info)
{
  float *data = getCache(info);
  m_caches.erase(info->op_key);
  return data;
}

size_t MemoryCache::getMaxBytes(const CompositorContext *ctx)
{
  return ctx->getMemCacheBytes();
}