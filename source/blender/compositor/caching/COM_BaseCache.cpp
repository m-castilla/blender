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

#include "BLI_assert.h"
#include <functional>

#include "COM_BaseCache.h"
#include "COM_BufferUtil.h"
#include "COM_CacheOperation.h"
#include "COM_CompositorContext.h"
#include "COM_TimeUtil.h"

RemovedCache::~RemovedCache()
{
  BufferUtil::hostFree(data);
}

size_t BaseCache::CacheInfo::getTotalBytes() const
{
  return BufferUtil::calcStdBufferBytes(op_key.op_width, op_key.op_height);
}

BaseCache::BaseCache(size_t op_type_hash)
    : m_op_type_hash(op_type_hash),
      m_max_bytes(0),
      m_current_bytes(0),
      m_caches(),
      m_caches_by_time(),
      m_prefetch_queue(),
      m_op_mode(OperationMode::Optimize)
{
}

BaseCache::~BaseCache()
{
  for (auto &it : m_caches) {
    delete it.second;
  }
  m_caches.clear();
  m_caches_by_time.clear();
}

void BaseCache::setOperationMode(OperationMode mode)
{
  m_op_mode = mode;
  if (mode == OperationMode::Exec) {
    // on exec mode start let's prefecth next cache if there is any
    prefetchNextCache();
  }
}

std::vector<std::shared_ptr<RemovedCache>> BaseCache::checkCaches(bool delete_caches)
{
  std::vector<std::shared_ptr<RemovedCache>> removed_caches;
  // assure the sum of all caches bytes are within global max, deleting oldest caches if
  // necessary (except last one)
  while (m_current_bytes > m_max_bytes && m_caches.size() > 1) {
    auto oldest_cache = m_caches.begin()->second;
    BLI_assert(m_current_bytes >= oldest_cache->getTotalBytes());
    m_current_bytes -= oldest_cache->getTotalBytes();
    if (delete_caches) {
      deleteCache(oldest_cache);
    }
    else {
      float *cache_buf = removeCache(oldest_cache);
      auto cache_data = new RemovedCache();
      cache_data->data = cache_buf;
      cache_data->last_use_time = oldest_cache->last_use_time;
      cache_data->last_save_time = oldest_cache->last_save_time;
      cache_data->op_key = oldest_cache->op_key;
      removed_caches.emplace_back(cache_data);
    }
    deleteCacheInfo(oldest_cache);
  }
  BLI_assert(m_caches.size() == 1 || m_current_bytes <= m_max_bytes);
  return removed_caches;
}

void BaseCache::deleteCacheInfo(CacheInfo *info)
{
  // delete info in by-time container
  auto time_it = m_caches_by_time.find(info);
  if (time_it != m_caches_by_time.end() && (*time_it)->op_key == info->op_key) {
    m_caches_by_time.erase(time_it);
  }

  // delete info in cache container
  auto cache_it = m_caches.find(info->op_key);
  if (cache_it != m_caches.end()) {
    auto info = cache_it->second;
    delete info;
    m_caches.erase(cache_it);
  }
}

void BaseCache::initialize(const CompositorContext *ctx)
{
  m_prefetch_added.clear();
  m_prefetch_queue.clear();
  m_max_bytes = getMaxBytes(ctx);
}
void BaseCache::deinitialize(const CompositorContext *ctx)
{
  m_prefetch_added.clear();
  m_prefetch_queue.clear();
}

void BaseCache::cacheReadOptimize(const OpKey &op_key)
{
  // Only add the first read to the prefetch queue , because once cache is read on write,
  // cache will be kept in buffer manager for next reads.
  if (m_prefetch_added.find(op_key) == m_prefetch_added.end()) {
    m_prefetch_added.insert(op_key);
    m_prefetch_queue.push_back(op_key);
  }
}

void BaseCache::saveCache(const OpKey &op_key,
                          float *data,
                          std::function<void()> on_save_end,
                          uint64_t last_save_time,
                          uint64_t last_use_time)
{
  BLI_assert(m_op_mode == OperationMode::Exec);
  auto info = loadCacheInfo(op_key, last_save_time, last_use_time);
  saveCache(info, data, on_save_end);
}

float *BaseCache::getCache(const OpKey &op_key)
{
  BLI_assert(m_op_mode == OperationMode::Exec);
  BLI_assert(hasCache(op_key));
  auto info = getCacheInfo(op_key);
  return info ? getCache(info) : nullptr;
}

float *BaseCache::getCacheAndPrefetchNext(const OpKey &op_key)
{
  BLI_assert(m_op_mode == OperationMode::Exec);
  BLI_assert(hasCache(op_key));

  // check cache info and remove it from prefetch queue
  BLI_assert(getCacheInfo(op_key)->op_key == op_key);
  BLI_assert(m_prefetch_queue.front() == op_key);
  m_prefetch_queue.pop_front();

  prefetchNextCache();

  auto info = getCacheInfo(op_key);
  return info ? getCache(info) : nullptr;
}

void BaseCache::prefetchNextCache()
{
  if (m_prefetch_queue.size() > 0) {
    const auto &prefetch_key = m_prefetch_queue.front();
    BLI_assert(hasCache(prefetch_key));
    auto info = getCacheInfo(prefetch_key);
    BLI_assert(info);
    prefetchCache(info);
  }
}

bool BaseCache::hasCache(const OpKey &op_key)
{
  return m_caches.find(op_key) != m_caches.end();
}

void BaseCache::deleteAllCaches()
{
  m_caches_by_time.clear();
  for (auto &cache : m_caches) {
    auto info = cache.second;
    delete info;
  }
  m_caches.clear();
}

BaseCache::CacheInfo *BaseCache::getCacheInfo(const OpKey &key)
{
  auto found_it = m_caches.find(key);
  if (found_it == m_caches.end()) {
    BLI_assert("Should not happen");
    return nullptr;
  }
  else {
    auto info = found_it->second;
    info->last_use_time = TimeUtil::getNowNsTime();
    return info;
  }
}

BaseCache::CacheInfo *BaseCache::loadCacheInfo(const OpKey &op_key,
                                               uint64_t last_save_time,
                                               uint64_t last_use_time)
{
  BLI_assert(op_key.op_type_hash == m_op_type_hash);
  CacheInfo *info = nullptr;
  auto found_it = m_caches.find(op_key);
  auto now = TimeUtil::getNowNsTime();
  if (found_it == m_caches.end()) {
    info = new CacheInfo();
    info->op_key = op_key;
    info->last_use_time = last_use_time <= 0 ? now : last_use_time;
    info->last_save_time = last_save_time <= 0 ? now : last_save_time;
    m_caches.emplace(op_key, info);
    m_caches_by_time.insert(info);
    m_current_bytes += info->getTotalBytes();
  }
  else {
    info = found_it->second;
    info->last_use_time = last_use_time <= 0 ? now : last_use_time;
    info->last_save_time = last_save_time <= 0 ? now : last_save_time;
  }
  return info;
}