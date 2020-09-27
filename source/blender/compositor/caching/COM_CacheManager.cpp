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

#include "COM_CacheManager.h"
#include "COM_BufferRecycler.h"
#include "COM_BufferUtil.h"
#include "COM_CacheOperation.h"
#include "COM_CompositorContext.h"
#include "COM_MathUtil.h"
#include "COM_Rect.h"

CacheManager::CacheManager()
    : m_disk_cache(new DiskCache(typeid(CacheOperation).hash_code())),
      m_mem_cache(new MemoryCache(typeid(CacheOperation).hash_code())),
      m_view_cache_man(),
      m_persistent_map(),
      m_ctx(nullptr),
      m_recycler(nullptr),
      m_last_get_cache_persist()
{
}

CacheManager::~CacheManager()
{
}

void CacheManager::initialize(const CompositorContext *ctx, BufferRecycler *recycler)
{
  m_ctx = ctx;
  m_recycler = recycler;
  m_mem_cache->initialize(ctx);
  m_disk_cache->initialize(ctx);
  m_view_cache_man.initialize();
  checkCaches();
}

void CacheManager::deinitialize(const CompositorContext *ctx)
{
  checkCaches();
  m_mem_cache->deinitialize(ctx);
  m_disk_cache->deinitialize(ctx);
  m_view_cache_man.deinitialize(ctx->isBreaked());
  m_ctx = nullptr;
}

void CacheManager::setOperationMode(OperationMode mode)
{
  m_mem_cache->setOperationMode(mode);
  m_disk_cache->setOperationMode(mode);
}

void CacheManager::checkCaches()
{
  auto mem_removed_caches = m_mem_cache->checkCaches(false);
  m_disk_cache->checkCaches(true);
  if (mem_removed_caches.size() > 0) {
    for (auto &cache : mem_removed_caches) {
      // on_save_end just retains the removed cache so that it's not deleted until save cache
      // finishes
      auto on_save_end = [=] {
        auto c = cache.get();
        c->last_use_time = 0;
      };
      m_disk_cache->saveCache(
          cache->op_key, cache->data, on_save_end, cache->last_save_time, cache->last_use_time);
    }
    m_disk_cache->checkCaches(true);
  }
}

PersistentKey CacheManager::buildPersistentKey(NodeOperation *op)
{
  PersistentKey pkey;
  pkey.n_frame = m_ctx->getCurrentFrame();
  pkey.node_id = op->getNodeId();
  pkey.op_width = op->getWidth();
  pkey.op_height = op->getHeight();
  pkey.op_data_type = op->getOutputDataType();

  pkey.hash = std::hash<int>()(pkey.n_frame);
  MathUtil::hashCombine(pkey.hash, pkey.node_id);
  MathUtil::hashCombine(pkey.hash, pkey.op_width);
  MathUtil::hashCombine(pkey.hash, pkey.op_height);
  MathUtil::hashCombine(pkey.hash, (int)pkey.op_data_type);

  return std::move(pkey);
}

std::pair<bool, OpKey> CacheManager::checkPersistentOpKey(NodeOperation *op)
{
  BLI_assert(isCacheable(op));
  bool is_persitent = isCacheableAndPersistent(op);
  PersistentKey pkey = buildPersistentKey(op);
  OpKey key;
  bool persistent_found = false;
  if (is_persitent) {
    auto found = m_persistent_map.find(pkey);
    if (found != m_persistent_map.end()) {
      if (hasCache(found->second)) {
        key = found->second;
        persistent_found = true;
      }
    }
  }

  if (!persistent_found) {
    // assure pkey is removed (the operation is not persistent anymore or cache has
    // been removed for being old)
    m_persistent_map.erase(pkey);
  }

  return std::make_pair(persistent_found, key);
}

TmpBuffer *CacheManager::getCachedOrNewAndPrefetchNext(NodeOperation *op)
{
  auto tmp = getCache(op, true);
  BLI_assert(!tmp || tmp->host.state == HostMemoryState::FILLED);
  if (!tmp) {
    tmp = BufferUtil::createStdTmpBuffer(
              nullptr, false, op->getWidth(), op->getHeight(), op->getOutputNUsedChannels())
              .release();
    BufferUtil::hostStdAlloc(tmp, op->getWidth(), op->getHeight());
    m_mem_cache->saveCache(op->getKey(), tmp->host.buffer);
    if (isCacheableAndPersistent(op)) {
      auto pkey = buildPersistentKey(op);
      m_persistent_map.insert_or_assign(pkey, op->getKey());
    }
    BLI_assert(tmp->host.state == HostMemoryState::CLEARED);
  }

  return tmp;
}

TmpBuffer *CacheManager::getCache(NodeOperation *op, bool prefetch_next)
{
  BLI_assert(isCacheable(op));

  OpKey key = op->getKey();

  float *cache_data = nullptr;
  bool is_recyclable = false;
  if (m_mem_cache->hasCache(key)) {
    cache_data = prefetch_next ? m_mem_cache->getCacheAndPrefetchNext(key) :
                                 m_mem_cache->getCache(key);
    is_recyclable = m_mem_cache->doesGetCacheReturnsCopy();
  }
  else if (m_ctx->useDiskCache() && m_disk_cache->hasCache(key)) {
    cache_data = prefetch_next ? m_disk_cache->getCacheAndPrefetchNext(key) :
                                 m_disk_cache->getCache(key);
    is_recyclable = m_disk_cache->doesGetCacheReturnsCopy();
  }

  if (cache_data) {
    auto tmp = BufferUtil::createStdTmpBuffer(
                   cache_data, true, op->getWidth(), op->getHeight(), op->getOutputNUsedChannels())
                   .release();
    if (is_recyclable) {
      tmp->is_host_recyclable = is_recyclable;
      m_recycler->setBufferAsTakenRecycle(tmp);
    }

    BLI_assert(!tmp || tmp->host.state == HostMemoryState::FILLED);
    return tmp;
  }
  else {
    // if cache could not be retrieved assure persistent key is removed
    if (isCacheableAndPersistent(op)) {
      auto pkey = buildPersistentKey(op);
      m_persistent_map.erase(pkey);
    }
    return nullptr;
  }
}

void CacheManager::cacheReadOptimize(NodeOperation *op)
{
  const OpKey &key = op->getKey();
  if (m_mem_cache->hasCache(key)) {
    m_mem_cache->cacheReadOptimize(key);
  }
  else if (m_ctx->useDiskCache() && m_disk_cache->hasCache(key)) {
    m_disk_cache->cacheReadOptimize(key);
  }
}
bool CacheManager::hasCache(NodeOperation *op)
{
  if (isCacheable(op)) {
    const OpKey key = op->getKey();
    return hasCache(key);
  }
  else {
    return false;
  }
}
bool CacheManager::hasCache(const OpKey &key)
{
  return (m_mem_cache->hasCache(key) || (m_ctx->useDiskCache() && m_disk_cache->hasCache(key)));
}

bool CacheManager::hasAnyKindOfCache(NodeOperation *op)
{
  return hasCache(op) || m_view_cache_man.hasViewCache(op);
}

bool CacheManager::isCacheable(NodeOperation *op)
{
  return typeid(*op) == typeid(CacheOperation);
}

bool CacheManager::isCacheableAndPersistent(NodeOperation *op)
{
  return isCacheable(op) && ((CacheOperation *)op)->isPersistent();
}
