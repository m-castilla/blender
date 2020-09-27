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
#include "BLI_path_util.h"
#include <chrono>
#include <filesystem>
#include <fstream>

#include "COM_BufferUtil.h"
#include "COM_CompositorContext.h"
#include "COM_DiskCache.h"
#include "COM_StringUtil.h"
#include "COM_TimeUtil.h"

const char FILENAME_PARTS_DELIMITER = '_';

namespace fs = std::filesystem;
DiskCache::DiskCache(size_t op_type_hash)
    : BaseCache(op_type_hash),
      m_cache_dir_path(""),
      m_load_thread(),
      m_prefetch_key(),
      m_prefetched_cache(nullptr),
      m_prefetch_thread(),
      m_save_threads(),
      m_delete_threads(),
      m_cache_dir_set(false),
      m_base_convert()
{
  deleteAllCaches();
}

DiskCache::~DiskCache()
{
  joinAllThreads();
  // if prefetched cache has not been gotten (when gotten is set to nullptr). Just free it.
  assurePreviousPrefetchFreed();

  deleteAllCaches();
}

static void printException(std::string msg_start, std::exception e)
{
  auto what = std::string(e.what());
  auto msg = msg_start + what;
  printf(msg.c_str());
}

void DiskCache::initialize(const CompositorContext *ctx)
{
  BaseCache::initialize(ctx);

  // set cache path
  const char *c_base_dir = ctx->getDiskCacheDir();
  if (c_base_dir && c_base_dir[0] != '\0') {
    char base_dir[FILE_MAX];
    strcpy(base_dir, c_base_dir);
    BLI_path_normalize(nullptr, base_dir);
    BLI_path_slash_ensure(base_dir);
    BLI_path_append(base_dir, FILE_MAX, CACHE_INNER_DIR_NAME);
    BLI_path_slash_ensure(base_dir);

    std::string user_cache_dir = std::string(base_dir);
    if (!m_cache_dir_set || user_cache_dir != m_cache_dir_path) {
      deleteAllCaches();

      try {
        if (!fs::exists(user_cache_dir)) {
          fs::create_directory(user_cache_dir);
        }
        m_cache_dir_path = user_cache_dir;
        m_cache_dir_set = true;
      }
      catch (const std::exception &exc) {
        printException(
            "Error creating compositor disk cache folder \"" + m_cache_dir_path + "\": ", exc);
        BLI_assert(!"Couldn't create compositor disk cache folder");
        m_cache_dir_set = false;
      }

      if (m_cache_dir_set) {
        loadCacheDir();
      }
    }
  }
  else {
    m_cache_dir_set = false;
  }
}

void DiskCache::deinitialize(const CompositorContext *ctx)
{
  // if prefetched cache has not been gotten (when gotten is set to nullptr). Just free it.
  assurePreviousPrefetchFreed();
  BaseCache::deinitialize(ctx);
}

void DiskCache::deleteAllCaches()
{
  if (m_cache_dir_set) {
    BaseCache::deleteAllCaches();
    joinAllThreads();
    try {
      if (fs::exists(m_cache_dir_path)) {
        std::filesystem::remove_all(m_cache_dir_path);
      }
    }
    catch (const std::exception &exc) {
      printException("Error deleting compositor disk cache folder \"" + m_cache_dir_path + "\": ",
                     exc);
      BLI_assert(!"Error deleting compositor disk cache folder");
    }
  }
}

void DiskCache::saveCache(const CacheInfo *info, float *data, std::function<void()> on_save_end)
{
  if (m_cache_dir_set) {
    joinRelatedThreads(info);

    auto op_key = info->op_key;
    auto total_bytes = info->getTotalBytes();
    auto file_path = getFilePath(info);
    std::function task = [=]() {
      try {
        auto myfile = std::fstream(file_path, std::ios::out | std::ios::binary);
        myfile.write((char *)data, total_bytes);
        myfile.close();
        if (on_save_end) {
          on_save_end();
        }
        BLI_assert(fs::file_size(file_path) == total_bytes);
      }
      catch (const std::exception &exc) {
        printException("Error saving compositor disk cache file \"" + file_path + "\": ", exc);
        BLI_assert(!"Error saving compositor disk cache");
      }
      m_save_ended_mutex.lock();
      m_save_ended_threads.insert(op_key);
      m_save_ended_mutex.unlock();
    };
    m_save_threads.insert(std::make_pair(op_key, std::thread(task)));
  }
}

void DiskCache::prefetchCache(const CacheInfo *info)
{
  if (m_cache_dir_set) {

    joinRelatedThreads(info);

    assurePreviousPrefetchFreed();

    m_prefetch_key = info->op_key;
    auto total_bytes = info->getTotalBytes();
    auto file_path = getFilePath(info);
    BLI_assert(fs::exists(file_path));
    BLI_assert(fs::file_size(file_path) == total_bytes);
    std::function task = [=]() {
      try {
        auto myfile = std::fstream(file_path, std::ios::in | std::ios::binary);
        m_prefetched_cache = BufferUtil::hostAlloc(total_bytes);
        BLI_assert(myfile.tellg() == 0);
        myfile.read((char *)m_prefetched_cache, total_bytes);
        myfile.close();
      }
      catch (const std::exception &exc) {
        printException("Error reading compositor disk cache file \"" + file_path + "\": ", exc);
        BLI_assert(!"Error reading compositor disk cache");
      }
    };

    if (m_prefetch_thread.joinable()) {
      m_prefetch_thread.join();
    }
    m_prefetch_thread = std::thread(task);
  }
}

float *DiskCache::getCache(const CacheInfo *info)
{
  if (m_cache_dir_set) {
    if (m_prefetch_thread.joinable()) {
      m_prefetch_thread.join();
    }
    auto prefetch = m_prefetched_cache;
    m_prefetched_cache = nullptr;
    return prefetch;
  }
  else {
    return nullptr;
  }
}

void DiskCache::deleteCache(const CacheInfo *info)
{
  if (m_cache_dir_set) {
    joinRelatedThreads(info);

    auto op_key = info->op_key;
    auto file_path = getFilePath(info);
    BLI_assert(fs::exists(file_path));
    std::function task = [=]() {
      try {
        std::filesystem::remove(file_path);
      }
      catch (const std::exception &exc) {
        printException("Error deleting compositor disk cache file \"" + file_path + "\": ", exc);
        BLI_assert(!"Error deleting compositor disk cache");
      }
      m_delete_ended_mutex.lock();
      m_delete_ended_threads.insert(op_key);
      m_delete_ended_mutex.unlock();
    };
    m_delete_threads.insert(std::make_pair(op_key, std::thread(task)));
  }
}

float *DiskCache::removeCache(const CacheInfo *info)
{
  // removeCache must not be called for non-memory based caches
  throw std::bad_function_call();
}

size_t DiskCache::getMaxBytes(const CompositorContext *ctx)
{
  return ctx->getDiskCacheBytes();
}

void DiskCache::loadCacheDir()
{
  fs::directory_iterator end_itr;  // default construction yields past-the-end
  try {
    for (fs::directory_iterator itr(m_cache_dir_path); itr != end_itr; ++itr) {
      if (fs::is_regular_file(itr->status())) {
        auto cache_info = getCacheInfoFromFilename(itr->path().filename().string());
        if (std::get<0>(cache_info)) {
          uint64_t last_save_time = std::get<2>(cache_info);
          // we intentionally use last_save_time as last_use_time on loading. As we rather avoid
          // having to change filename or updating the file each time cache is used.
          loadCacheInfo(std::get<1>(cache_info), last_save_time, last_save_time);
        }
      }
    }
  }
  catch (const std::exception &exc) {
    printException("Error checking disk cache files: ", exc);
    BLI_assert(!"Error checking compositor disk cache files");
  }
}

void DiskCache::assurePreviousPrefetchFreed()
{
  // if previously prefetched cache has not been gotten (when gotten is set to nullptr). Just
  // free it.
  if (m_prefetch_thread.joinable()) {
    m_prefetch_thread.join();
  }
  if (m_prefetched_cache) {
    BufferUtil::hostFree(m_prefetched_cache);
    m_prefetched_cache = nullptr;
  }
}

void DiskCache::joinRelatedThreads(const CacheInfo *info)
{
  if (m_load_thread.joinable()) {
    m_load_thread.join();
  }

  OpKey key = info->op_key;
  if (m_prefetch_key == key && m_prefetch_thread.joinable()) {
    m_prefetch_thread.join();
  }

  {
    auto save_it = m_save_threads.find(key);
    if (save_it != m_save_threads.end()) {
      auto &thread = save_it->second;
      if (thread.joinable()) {
        thread.join();
      }
    }
  }
  {
    auto delete_it = m_delete_threads.find(key);
    if (delete_it != m_delete_threads.end()) {
      auto &thread = delete_it->second;
      if (thread.joinable()) {
        thread.join();
      }
    }
  }
  removeEndedThreads();
}

void DiskCache::joinAllThreads()
{
  if (m_load_thread.joinable()) {
    m_load_thread.join();
  }

  if (m_prefetch_thread.joinable()) {
    m_prefetch_thread.join();
  }
  for (auto &entry : m_save_threads) {
    auto &thread = entry.second;
    if (thread.joinable()) {
      thread.join();
    }
  }
  for (auto &entry : m_delete_threads) {
    auto &thread = entry.second;
    if (thread.joinable()) {
      thread.join();
    }
  }
  removeEndedThreads();
}

static void removeThreads(std::unordered_map<OpKey, std::thread> &threads_map,
                          std::unordered_set<OpKey> &ended_set)
{
  for (const auto &op_key : ended_set) {
    auto found_it = threads_map.find(op_key);
    if (found_it != threads_map.end()) {
      auto &thread = found_it->second;
      if (thread.joinable()) {
        thread.join();
      }
      threads_map.erase(op_key);
    }
  }
  ended_set.clear();
}

void DiskCache::removeEndedThreads()
{
  m_save_ended_mutex.lock();
  removeThreads(m_save_threads, m_save_ended_threads);
  m_save_ended_mutex.unlock();

  m_delete_ended_mutex.lock();
  removeThreads(m_delete_threads, m_delete_ended_threads);
  m_delete_ended_mutex.unlock();
}

const int FILENAME_N_PARTS = 5;
std::string DiskCache::getFilePath(const CacheInfo *cache_info)
{
  const auto &op_key = cache_info->op_key;
  auto width_str = m_base_convert.convertBase10ToBase(op_key.op_width, BaseConverter::MAX_BASE);
  auto height_str = m_base_convert.convertBase10ToBase(op_key.op_height, BaseConverter::MAX_BASE);
  auto datatype_str = m_base_convert.convertBase10ToBase((size_t)op_key.op_data_type,
                                                         BaseConverter::MAX_BASE);
  auto save_time_str = m_base_convert.convertBase10ToBase(cache_info->last_save_time,
                                                          BaseConverter::MAX_BASE);
  auto hash_str = m_base_convert.convertBase10ToBase(op_key.op_hash, BaseConverter::MAX_BASE);
  std::string filename = width_str + FILENAME_PARTS_DELIMITER + height_str +
                         FILENAME_PARTS_DELIMITER + datatype_str + FILENAME_PARTS_DELIMITER +
                         hash_str + FILENAME_PARTS_DELIMITER + save_time_str;
  return m_cache_dir_path + filename;
}

std::tuple<bool, OpKey, uint64_t> DiskCache::getCacheInfoFromFilename(const std::string filename)
{
  OpKey key;
  uint64_t save_time = 0;
  auto parts = StringUtil::split(filename, FILENAME_PARTS_DELIMITER);
  if (parts.size() == FILENAME_N_PARTS) {
    size_t results[FILENAME_N_PARTS];
    for (int i = 0; i < FILENAME_N_PARTS; i++) {
      auto result = m_base_convert.convertBaseToBase10(parts[i], BaseConverter::MAX_BASE);
      if (result.first) {
        results[i] = result.second;
      }
      else {
        return std::make_tuple(false, key, save_time);
      }
    }
    key.op_width = results[0];
    key.op_height = results[1];
    key.op_data_type = (DataType)results[2];
    key.op_hash = results[3];
    key.op_type_hash = getCacheOpTypeHash();
    save_time = results[4];
    return std::make_tuple(true, key, save_time);
  }
  else {
    return std::make_tuple(false, key, save_time);
  }
}
