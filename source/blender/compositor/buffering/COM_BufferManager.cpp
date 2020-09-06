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

#include "COM_BufferManager.h"
#include "BLI_assert.h"
#include "BLI_rect.h"
#include "BLI_utildefines.h"
#include "COM_BrightnessOperation.h"
#include "COM_Buffer.h"
#include "COM_BufferRecycler.h"
#include "COM_BufferUtil.h"
#include "COM_CompositorContext.h"
#include "COM_ComputeDevice.h"
#include "COM_ExecutionManager.h"
#include "COM_NodeOperation.h"
#include "COM_PixelsUtil.h"
#include "COM_ReadsOptimizer.h"
#include "COM_RectUtil.h"
#include "COM_defines.h"
#include <algorithm>
#include <ctime>

const size_t CLEAN_CACHE_STEP_BYTES = 64 * 1024 * 1024;  // 64MB
                                                         /* Pass arguments into this contructor*/
using namespace std::placeholders;
BufferManager::BufferManager()
    : m_initialized(false),
      m_optimizers(),
      m_cached_buffers(),
      m_recycler(),
      m_readers_reads(),
      m_reads_gotten(false),
      m_max_cache_bytes(0),
      m_current_cache_bytes(0)
{
}

BufferManager::~BufferManager()
{
  deinitialize(false);
  for (const auto &cache : m_cached_buffers) {
    BufferUtil::deleteCacheBuffer(cache.second);
  }
}

void BufferManager::initialize(const CompositorContext &context)
{
  if (!m_initialized) {

    if (!m_recycler) {
      m_recycler = std::unique_ptr<BufferRecycler>(new BufferRecycler());
    }
    m_recycler->setExecutionId(context.getExecutionId());

    m_max_cache_bytes = context.getBufferCacheSize();
    BLI_assert(m_max_cache_bytes >= CLEAN_CACHE_STEP_BYTES);

    checkCache();

    m_initialized = true;
  }
}

void BufferManager::deinitialize(bool isBreaked)
{
  if (m_initialized) {
    if (!isBreaked) {
      m_recycler->assertRecycledEqualsCreatedBuffers();
    }

    // trying to recycle buffers between executions is giving issues. Disable it right now.
#if 0
    m_recycler->recycleCurrentExecNonRecycledBuffers();
    m_recycler->deleteNonRecycledBuffers();
#else
    m_recycler->deleteAllBuffers();
#endif

#if defined(DEBUG)
    if (!isBreaked) {
      // assert all reads counted during optimization are completed on exec
      for (auto &entry : m_readers_reads) {
        auto reads_list = entry.second;
        for (auto reader_reads : reads_list) {
          auto reads = reader_reads->reads;
          BLI_assert(reads->current_cpu_reads == reads->total_cpu_reads);
          BLI_assert(reads->current_compute_reads == reads->total_compute_reads);
        }
      }
    }
#endif

    m_readers_reads.clear();
    m_reads_gotten = false;

    for (const auto &opti_entry : m_optimizers) {
      delete opti_entry.second;
    }
    m_optimizers.clear();

    m_initialized = false;
  }
}

/* check if current cache size is greater than max, if so we delete the oldest used caches until
 size is under maximum */
void BufferManager::checkCache()
{
  while (m_current_cache_bytes > m_max_cache_bytes && m_cached_buffers.size() > 1) {
    size_t desired_bytes = CLEAN_CACHE_STEP_BYTES > m_max_cache_bytes ?
                               0 :
                               m_max_cache_bytes - CLEAN_CACHE_STEP_BYTES;
    std::vector<std::pair<OpKey, CacheBuffer *>> caches_by_time(m_cached_buffers.begin(),
                                                                m_cached_buffers.end());
    std::sort(caches_by_time.begin(),
              caches_by_time.end(),
              [&](std::pair<OpKey, CacheBuffer *> entry1, std::pair<OpKey, CacheBuffer *> entry2) {
                return entry1.second->last_use_time < entry2.second->last_use_time;
              });

    // we never delete the last cache added, would be undesirable for the user (end() - 1)
    for (auto it = caches_by_time.begin(); it != caches_by_time.end() - 1; it++) {
      if (m_current_cache_bytes > desired_bytes) {
        CacheBuffer *cache = it->second;
        HostBuffer host = cache->host;
        m_current_cache_bytes -= cache->height * host.brow_bytes;
        BufferUtil::deleteCacheBuffer(it->second);
        m_cached_buffers.erase(it->first);
      }
      else {
        break;
      }
    }
  }
}

static std::shared_ptr<PixelsRect> tmpPixelsRect(NodeOperation *op,
                                                 const rcti &op_rect,
                                                 OpReads *reads,
                                                 bool is_for_read)
{
  int width = BLI_rcti_size_x(&op_rect);
  int height = BLI_rcti_size_y(&op_rect);
  int offset_x = op_rect.xmin;
  int offset_y = op_rect.ymin;
  PixelsRect *r;
  if (op->isSingleElem()) {
    r = new PixelsRect(is_for_read ? op->getSingleElem() : (float *)nullptr,
                       COM_NUM_CHANNELS_STD,
                       offset_x,
                       offset_x + width,
                       offset_y,
                       offset_y + height);
  }
  else {
    r = new PixelsRect(reads->tmp_buffer, offset_x, offset_x + width, offset_y, offset_y + height);
  }
  return std::shared_ptr<PixelsRect>(r);
}

void BufferManager::readOptimize(NodeOperation *op,
                                 NodeOperation *reader_op,
                                 ExecutionManager &man)
{
  ReadsOptimizer *optimizer;
  auto key = op->getKey();

  auto find_it = m_optimizers.find(key);
  if (find_it == m_optimizers.end()) {
    optimizer = new ReadsOptimizer();
    m_optimizers.insert({key, optimizer});
  }
  else {
    optimizer = m_optimizers[key];
  }

  optimizer->optimize(op, reader_op, man);
}

BufferManager::ReadResult BufferManager::readSeek(NodeOperation *op, ExecutionManager &man)
{
  ReadResult result = ReadResult{false, {}};
  const auto &key = op->getKey();
  auto optimizer_found = m_optimizers.find(key);
  if (optimizer_found == m_optimizers.end()) {
    // this operation getPixels() has not beed called during optimization phase. Probably an input
    // constant
    result.is_written = true;
  }
  else {
    auto optimizer = optimizer_found->second;
    auto reads = optimizer->peepReads(man);

    if (op->getBufferType() == BufferType::CACHED) {
      CacheBuffer *cache = getCache(op);
      BLI_assert(cache);
      if (cache->host.state == HostMemoryState::FILLED) {
        if (!reads->is_write_complete) {
          reads->is_write_complete = true;
          reportWriteCompleted(op, reads, man);
        }
        reads->tmp_buffer = m_recycler->createStdTmpBuffer(
            false, nullptr, op->getWidth(), op->getHeight(), op->getOutputNUsedChannels());
        reads->tmp_buffer->host = cache->host;
        ASSERT_VALID_STD_TMP_BUFFER(reads->tmp_buffer, cache->width, cache->height);
      }
      else {
        BLI_assert(cache->host.state == HostMemoryState::CLEARED);
        BLI_assert(!reads->is_write_complete);
      }
    }
    else if (op->getBufferType() == BufferType::NO_BUFFER_NO_WRITE) {
      if (!reads->is_write_complete) {
        reads->is_write_complete = true;
        reportWriteCompleted(op, reads, man);
      }
    }

    if (reads->is_write_complete) {
      result.is_written = true;
      rcti full_rect;
      BLI_rcti_init(&full_rect, 0, reads->readed_op->getWidth(), 0, reads->readed_op->getHeight());
      auto pixels = tmpPixelsRect(op, full_rect, reads, true);
      result.pixels.swap(pixels);
    }
  }

  return result;
}

void BufferManager::writeSeek(NodeOperation *op,
                              ExecutionManager &man,
                              std::function<void(TmpRectBuilder &)> write_func)
{
  const auto &key = op->getKey();

  BLI_assert(m_optimizers.find(key) != m_optimizers.end());
  auto reads = m_optimizers[key]->peepReads(man);
  if (!reads->is_write_complete) {
    if (!reads->is_write_complete) {
      BLI_assert(reads->tmp_buffer == NULL);

      bool do_write = true;
      bool is_write_computed = op->isComputed(man);
      if (op->getBufferType() == BufferType::CACHED) {
        CacheBuffer *cache = getCache(op);
        BLI_assert(cache);
        // write cache should only be called when it's cleared because if filled it should be
        // returned on readSeek
        BLI_assert(cache->host.state == HostMemoryState::CLEARED);
        reads->tmp_buffer = m_recycler->createStdTmpBuffer(
            false, nullptr, op->getWidth(), op->getHeight(), op->getOutputNUsedChannels());
        reads->tmp_buffer->host = cache->host;
      }
      else if (op->getBufferType() == BufferType::CUSTOM) {
        reads->tmp_buffer = getCustomBuffer(op);
        do_write = false;
        is_write_computed = false;
      }
      else if (op->getBufferType() == BufferType::NO_BUFFER_WITH_WRITE) {
        // no tmp mem_buf needed, writing is done to an output resource and there should be no
        // reads
      }
      else if (op->getBufferType() == BufferType::NO_BUFFER_NO_WRITE) {
        do_write = false;
        is_write_computed = false;
      }
      else if (op->getBufferType() == BufferType::TEMPORAL) {
        reads->tmp_buffer = m_recycler->createTmpBuffer(true);
      }
      else {
        BLI_assert(!"Non implemented BufferType");
      }

      bool compute_work_enqueued = false;
      // for writing that has no buffer there is no need to prepare write buffers for
      // either write or reading. For the others even when write is not needed, it must be called
      // for buffers preparation before calling prepareForRead
      if (BufferUtil::hasBuffer(op->getBufferType()) && !man.isBreaked()) {
        compute_work_enqueued |= prepareForWrite(is_write_computed, reads);
      }
      if (compute_work_enqueued) {
        man.deviceWaitQueueToFinish();
        compute_work_enqueued = false;
      }

      if (do_write && !man.isBreaked()) {
        TmpRectBuilder tmp_rect_builder = std::bind(&tmpPixelsRect, op, _1, reads, false);
        write_func(tmp_rect_builder);
        if (is_write_computed) {
          compute_work_enqueued = true;
        }

        if (BufferUtil::hasBuffer(op->getBufferType())) {
          auto buf = reads->tmp_buffer;
          if (is_write_computed) {
            buf->device.state = DeviceMemoryState::FILLED;
          }
          else if (buf->host.state != HostMemoryState::MAP_FROM_DEVICE) {
            buf->host.state = HostMemoryState::FILLED;
          }
        }
      }

      if (!man.isBreaked()) {
        if (compute_work_enqueued) {
          man.deviceWaitQueueToFinish();
          compute_work_enqueued = false;
        }
        reportWriteCompleted(op, reads, man);

        if (BufferUtil::hasBuffer(op->getBufferType())) {
          compute_work_enqueued = prepareForRead(is_write_computed, reads);
        }
        if (compute_work_enqueued) {
          man.deviceWaitQueueToFinish();
          compute_work_enqueued = false;
        }
      }

      reads->is_write_complete = true;
      man.reportOperationCompleted(op);
    }
  }
}

/* returns whether there has been enqueued work on device */
bool BufferManager::prepareForWrite(bool is_write_computed, OpReads *reads)
{
  bool work_enqueued = false;
  auto buf = reads->tmp_buffer;
  int width = reads->readed_op->getWidth();
  int height = reads->readed_op->getHeight();
  int elem_chs = reads->readed_op->getOutputNUsedChannels();
  bool host_ready = buf->host.state == HostMemoryState::CLEARED ||
                    buf->host.state == HostMemoryState::FILLED;
  bool host_empty = buf->host.state == HostMemoryState::NONE;
  bool device_empty = buf->device.state == DeviceMemoryState::NONE;
  (void)host_empty;    // removes warnings
  (void)device_empty;  // removes warnings
  if (reads->total_compute_reads > 0 && reads->total_cpu_reads > 0) {
    BLI_assert(device_empty);
    if (is_write_computed) {
      work_enqueued |= m_recycler->takeStdRecycle(
          BufferRecycleType::DEVICE_CLEAR, buf, width, height, elem_chs);
    }
    else if (!host_ready) {
      BLI_assert(host_empty);
      work_enqueued |= m_recycler->takeStdRecycle(
          BufferRecycleType::HOST_CLEAR, buf, width, height, elem_chs);
    }
  }
  else {
    BufferRecycleType recycle_type = BufferRecycleType::HOST_CLEAR;
    bool take_recycle = true;
    if (reads->total_compute_reads > 0) {
      if (is_write_computed) {
        BLI_assert(device_empty && host_empty);
        recycle_type = BufferRecycleType::DEVICE_CLEAR;
      }
      else if (!host_ready) {
        BLI_assert(host_empty);
        recycle_type = BufferRecycleType::DEVICE_HOST_MAPPED;
      }
      else {
        take_recycle = false;
      }
    }
    else if (reads->total_cpu_reads >= 0) {
      // total_compute_reads==0 && total_cpu_reads==0 case should only happen for outputs that
      // might want to temporarily write to a buffer.
      if (is_write_computed) {
        BLI_assert(device_empty && host_empty);
        recycle_type = BufferRecycleType::DEVICE_HOST_ALLOC;
      }
      else if (!host_ready) {
        BLI_assert(host_empty);
        recycle_type = BufferRecycleType::HOST_CLEAR;
      }
      else {
        take_recycle = false;
      }
    }
    else {
      BLI_assert(!"Should never happen");
    }

    if (take_recycle) {
      work_enqueued |= m_recycler->takeStdRecycle(recycle_type, buf, width, height, elem_chs);
      BLI_assert(recycle_type != BufferRecycleType::HOST_CLEAR ||
                 (buf->host.buffer != nullptr && buf->host.bwidth > 0 && buf->host.bheight > 0 &&
                  buf->host.belem_chs > 0));
      BLI_assert(recycle_type == BufferRecycleType::HOST_CLEAR ||
                 (buf->device.buffer != nullptr && buf->device.bwidth > 0 &&
                  buf->device.bheight > 0 && buf->device.belem_chs > 0));
    }
  }
  return work_enqueued;
}

/* returns whether there has been enqueued work on device */
bool BufferManager::prepareForRead(bool is_compute_written, OpReads *reads)
{
  bool work_enqueued = false;
  auto buf = reads->tmp_buffer;

  if (reads->total_compute_reads > 0 && reads->total_cpu_reads > 0) {
    if (is_compute_written) {
      BLI_assert(buf->device.state == DeviceMemoryState::FILLED);
      if (buf->host.state == HostMemoryState::NONE) {
        work_enqueued |= m_recycler->takeStdRecycle(
            BufferRecycleType::HOST_CLEAR, buf, buf->width, buf->height, buf->elem_chs);
      }
      BufferUtil::deviceToHostCopyEnqueue(buf);
    }
    else {
      BLI_assert(buf->device.state == DeviceMemoryState::NONE);
      BLI_assert(buf->host.state == HostMemoryState::FILLED);
      work_enqueued |= m_recycler->takeStdRecycle(
          BufferRecycleType::DEVICE_CLEAR, buf, buf->width, buf->height, buf->elem_chs);
      BufferUtil::hostToDeviceCopyEnqueue(buf);
    }
    work_enqueued = true;
  }
  else {
    if (reads->total_compute_reads > 0) {
      if (!is_compute_written) {
        if (buf->host.state == HostMemoryState::MAP_FROM_DEVICE) {
          BLI_assert(buf->device.state == DeviceMemoryState::MAP_TO_HOST &&
                     buf->device.has_map_alloc);
          BufferUtil::deviceUnmapFromHostEnqueue(buf);
        }
        else if (buf->host.state == HostMemoryState::FILLED) {
          BLI_assert(buf->device.state == DeviceMemoryState::NONE);
          work_enqueued |= m_recycler->takeStdRecycle(
              BufferRecycleType::DEVICE_CLEAR, buf, buf->width, buf->height, buf->elem_chs);
          BLI_assert(buf->device.state == DeviceMemoryState::CLEARED);
          BufferUtil::hostToDeviceCopyEnqueue(buf);
        }
        else {
          BLI_assert(!"Incorrect buffer state");
        }
        work_enqueued = true;
      }
      else {
        BLI_assert(buf->device.state == DeviceMemoryState::FILLED);
      }
    }
    else if (reads->total_cpu_reads > 0) {
      if (is_compute_written) {
        BLI_assert(buf->device.state == DeviceMemoryState::FILLED && buf->device.has_map_alloc &&
                   buf->host.state != HostMemoryState::MAP_FROM_DEVICE);
        BufferUtil::deviceMapToHostEnqueue(buf, MemoryAccess::READ_WRITE);
        work_enqueued = true;
      }
      else {
        BLI_assert(buf->host.state == HostMemoryState::FILLED);
      }
    }
    else {
      // If there is no reads, don't need to do anything
    }
  }
  return work_enqueued;
}

CacheBuffer *BufferManager::getCache(NodeOperation *op)
{
  BLI_assert(op->getBufferType() == BufferType::CACHED);
  CacheBuffer *cache = nullptr;
  auto key = op->getKey();

  auto find_cache_it = m_cached_buffers.find(key);
  if (find_cache_it != m_cached_buffers.end()) {
    cache = find_cache_it->second;
    cache->last_use_time = clock();
  }
  else {
    int n_used_chs = op->getOutputNUsedChannels();
    cache = new CacheBuffer();
    cache->height = op->getHeight();
    cache->width = op->getWidth();
    cache->elem_chs = n_used_chs;
    HostBuffer &host = cache->host;
    host.brow_bytes = BufferUtil::calcStdBufferRowBytes(cache->width);
    host.bwidth = op->getWidth();
    host.bheight = op->getHeight();
    host.belem_chs = n_used_chs;
    host.buffer = BufferUtil::hostAlloc(cache->width, cache->height, COM_NUM_CHANNELS_STD);
    host.state = HostMemoryState::CLEARED;

    m_cached_buffers.insert({key, cache});
    m_current_cache_bytes += cache->height * cache->host.brow_bytes;
    checkCache();
    cache->last_use_time = clock();
  }

  BLI_assert(op->getOutputNUsedChannels() == cache->elem_chs);
  BLI_assert(op->getHeight() == cache->height);
  BLI_assert(op->getWidth() == cache->width);
  return cache;
}

bool BufferManager::hasBufferCache(NodeOperation *op)
{
  return op->getBufferType() == BufferType::CACHED &&
         m_cached_buffers.find(op->getKey()) != m_cached_buffers.end();
}

TmpBuffer *BufferManager::getCustomBuffer(NodeOperation *op)
{
  BLI_assert(op->getBufferType() == BufferType::CUSTOM);
  int n_used_chs = op->getOutputNUsedChannels();
  TmpBuffer *custom = op->getCustomBuffer();
  BLI_assert(custom != nullptr && custom->host.buffer != nullptr);
  BLI_assert(custom->height == op->getHeight());
  BLI_assert(custom->width == op->getWidth());
  BLI_assert(custom->elem_chs == n_used_chs);
  BLI_assert(custom->host.bheight >= custom->height);
  BLI_assert(custom->host.bwidth >= custom->width);
  BLI_assert(custom->host.belem_chs >= n_used_chs);
  BLI_assert(custom->host.brow_bytes ==
             BufferUtil::calcNonStdBufferRowBytes(custom->host.bwidth, custom->host.belem_chs));
  BLI_assert(custom->host.state == HostMemoryState::CLEARED ||
             custom->host.state == HostMemoryState::FILLED);
  if (custom->host.state == HostMemoryState::CLEARED) {
    custom->host.state = HostMemoryState::FILLED;
  }

  UNUSED_VARS(n_used_chs);
  return custom;
}

void BufferManager::reportWriteCompleted(NodeOperation *op,
                                         OpReads *op_reads,
                                         ExecutionManager &man)
{
  if (op->getBufferType() == BufferType::CACHED) {
    getCache(op)->host.state = HostMemoryState::FILLED;
  }
  if (op_reads->tmp_buffer != nullptr && op_reads->total_compute_reads == 0 &&
      op_reads->total_cpu_reads == 0) {
    m_recycler->giveRecycle(op_reads->tmp_buffer);
    op_reads->tmp_buffer = nullptr;
  }

  assureReadsGotten(man);

  const OpKey &reader_key = op->getKey();
  auto reads_found = m_readers_reads.find(reader_key);
  if (reads_found != m_readers_reads.end()) {
    std::vector<ReaderReads *> &reads_list = reads_found->second;
    for (auto reader_reads : reads_list) {
      auto reads = reader_reads->reads;
      reads->current_compute_reads += reader_reads->n_compute_reads;
      reads->current_cpu_reads += reader_reads->n_cpu_reads;
      BLI_assert(reads->current_compute_reads <= reads->total_compute_reads);
      BLI_assert(reads->current_cpu_reads <= reads->total_cpu_reads);

      // operations with no buffer, need no recycle
      if (reads->tmp_buffer != nullptr &&
          reads->current_compute_reads == reads->total_compute_reads &&
          reads->current_cpu_reads == reads->total_cpu_reads) {
        m_recycler->giveRecycle(reads->tmp_buffer);
        reads->tmp_buffer = nullptr;
      }
    }
  }
}

const std::unordered_map<OpKey, std::vector<ReaderReads *>> *BufferManager::getReadersReads(
    ExecutionManager &man)
{
  assureReadsGotten(man);
  return &m_readers_reads;
}

void BufferManager::assureReadsGotten(ExecutionManager &man)
{
  if (!m_reads_gotten) {
    for (const auto &optimizer : m_optimizers) {
      auto receiver_key = optimizer.first;
      std::unordered_set<OpKey> receiver_reads;

      auto readers_reads = optimizer.second->peepAllReadersReads(man);
      for (const std::pair<OpKey, ReaderReads *> &reads : readers_reads) {
        const OpKey &reader_key = reads.first;

        auto reader_found = m_readers_reads.find(reader_key);
        if (reader_found == m_readers_reads.end()) {
          auto reader_reads = std::vector<ReaderReads *>();
          reader_reads.push_back(reads.second);
          m_readers_reads.insert({reader_key, std::move(reader_reads)});
        }
        else {
          auto &reader_reads = reader_found->second;
          reader_reads.push_back(reads.second);
        }

        receiver_reads.insert(reader_key);
      }

      m_received_reads.insert({receiver_key, std::move(receiver_reads)});
      m_reads_gotten = true;
    }
  }
}