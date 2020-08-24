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

#include "COM_BufferRecycler.h"
#include "BLI_assert.h"
#include "COM_BufferUtil.h"
#include "COM_ExecutionSystem.h"
#include "COM_RectUtil.h"

const float MAX_BUFFER_SCALE_DIFF_FOR_REUSE = 5.0f;
void ForeachBufferRecycleType(std::function<void(BufferRecycleType)> func)
{
  for (int i = 0; i <= static_cast<int>(BufferRecycleType::HOST_CLEAR); i++) {
    func(static_cast<BufferRecycleType>(i));
  }
}

BufferRecycler::BufferRecycler() : m_recycle(), m_created_buffers(), m_execution_id("")
{
  ForeachBufferRecycleType([&](BufferRecycleType type) {
    m_recycle.insert({type, new RecycleData()});
  });
}

void BufferRecycler::setExecutionId(const std::string &execution_id)
{
  m_execution_id = execution_id;
}

static void deleteBuffer(TmpBuffer *buf)
{
  if (buf->device.state != DeviceMemoryState::NONE) {
    BufferUtil::deviceFree(buf);
  }
  if (buf->is_host_recyclable) {
    if (buf->host.state != HostMemoryState::NONE) {
      BufferUtil::hostFree(buf);
    }
    if (buf->orig_host.state != HostMemoryState::NONE) {
      BufferUtil::origHostFree(buf);
    }
  }
  BLI_assert(buf->host.buffer == nullptr && buf->host.state == HostMemoryState::NONE);
  BLI_assert(buf->orig_host.buffer == nullptr && buf->orig_host.state == HostMemoryState::NONE);
  BLI_assert(buf->device.buffer == nullptr && buf->device.state == DeviceMemoryState::NONE);
  delete buf;
}

BufferRecycler::~BufferRecycler()
{
  deleteAllBuffers();
  ForeachBufferRecycleType([&](BufferRecycleType type) {
    RecycleData *rdata = m_recycle[type];
    delete rdata;
  });
}

void BufferRecycler::checkRecycledBufferCreated(TmpBuffer *recycled)
{
  if (recycled->host.buffer != nullptr &&
      recycled->host.state != HostMemoryState::MAP_FROM_DEVICE) {
    bool found = false;
    for (auto created : m_created_buffers) {
      if (created->host.buffer == recycled->host.buffer ||
          created->orig_host.buffer == recycled->host.buffer) {
        found = true;
        break;
      }
    }
    BLI_assert(found);
  }
  if (recycled->orig_host.buffer != nullptr) {
    bool found = false;
    for (auto created : m_created_buffers) {
      if (created->host.buffer == recycled->orig_host.buffer ||
          created->orig_host.buffer == recycled->orig_host.buffer) {
        found = true;
        break;
      }
    }
    BLI_assert(found);
  }
  if (recycled->device.buffer != nullptr) {
    bool found = false;
    for (auto created : m_created_buffers) {
      if (created->device.buffer == recycled->device.buffer) {
        found = true;
        break;
      }
    }
    BLI_assert(found);
  }
}

bool BufferRecycler::isCreatedBufferRecycled(TmpBuffer *created_buf)
{
  bool orig_buf_found = false;
  if (created_buf->orig_host.state != HostMemoryState::NONE) {
    RecycleData *rdata = m_recycle[BufferRecycleType::HOST_CLEAR];
    for (auto recycled_buf : rdata->buffers) {
      if (recycled_buf->host.state == HostMemoryState::CLEARED &&
          created_buf->orig_host.buffer == recycled_buf->host.buffer) {
        orig_buf_found = true;
        break;
      }
    }
  }
  else {
    orig_buf_found = true;
  }

  bool buf_found = false;
  if (created_buf->host.state == HostMemoryState::MAP_FROM_DEVICE) {
    RecycleData *rdata = m_recycle[BufferRecycleType::DEVICE_HOST_MAPPED];
    for (auto recycled_buf : rdata->buffers) {
      if (recycled_buf->host.state == HostMemoryState::MAP_FROM_DEVICE &&
          recycled_buf->device.state == DeviceMemoryState::MAP_TO_HOST &&
          created_buf->host.buffer == recycled_buf->host.buffer &&
          created_buf->device.buffer == recycled_buf->device.buffer) {
        buf_found = true;
        break;
      }
    }
  }
  else if (created_buf->host.state == HostMemoryState::CLEARED ||
           created_buf->host.state == HostMemoryState::FILLED) {
    RecycleData *rdata = m_recycle[BufferRecycleType::HOST_CLEAR];
    for (auto recycled_buf : rdata->buffers) {
      if (recycled_buf->host.state == HostMemoryState::CLEARED &&
          created_buf->host.buffer == recycled_buf->host.buffer) {
        buf_found = true;
        break;
      }
    }
  }
  else if (created_buf->device.state == DeviceMemoryState::CLEARED ||
           created_buf->device.state == DeviceMemoryState::FILLED) {
    auto recycle_type = created_buf->device.has_map_alloc ? BufferRecycleType::DEVICE_HOST_ALLOC :
                                                            BufferRecycleType::DEVICE_CLEAR;
    RecycleData *rdata = m_recycle[recycle_type];
    for (auto recycled_buf : rdata->buffers) {
      if (recycled_buf->device.state == DeviceMemoryState::CLEARED &&
          created_buf->device.buffer == recycled_buf->device.buffer) {
        buf_found = true;
        break;
      }
    }
  }
  else {
    BLI_assert("Unknown buffer state");
  }

  return orig_buf_found && buf_found;
}

void BufferRecycler::assertRecycledEqualsCreatedBuffers()
{
#if defined(DEBUG) || defined(COM_DEBUG)
  // assert created buffers are recycled
  for (auto created_buf : m_created_buffers) {
    BLI_assert(isCreatedBufferRecycled(created_buf));
  }

  // assert all recycled buffers are in created buffers list
  ForeachBufferRecycleType([&](BufferRecycleType type) {
    RecycleData *rdata = m_recycle[type];
    for (auto recycled : rdata->buffers) {
      checkRecycledBufferCreated(recycled);
    }
  });
#endif
}

void BufferRecycler::deleteAllBuffers()
{
  deleteBuffers(true);
}

void BufferRecycler::deleteNonRecycledBuffers()
{
  deleteBuffers(false);
}

void BufferRecycler::recycleCurrentExecNonRecycledBuffers()
{
  for (TmpBuffer *buf : m_created_buffers) {
    if (buf->execution_id == m_execution_id) {
      giveRecycle(buf);
    }
  }
}

void BufferRecycler::deleteBuffers(bool deleteRecycledBuffers)
{

  // first unmap all buffers
  bool needsWaitQueue = false;
  for (auto buf : m_created_buffers) {
    if (buf->host.state == HostMemoryState::MAP_FROM_DEVICE) {
      if (deleteRecycledBuffers || !isCreatedBufferRecycled(buf)) {
        BufferUtil::deviceUnmapFromHostEnqueue(buf);
        needsWaitQueue = true;
      }
    }
  }
  if (needsWaitQueue) {
    ExecutionManager::deviceWaitQueueToFinish();
  }

  // delete all non kept buffers
  for (auto it = m_created_buffers.begin(); it != m_created_buffers.end();) {
    auto buf = *it;
    if (deleteRecycledBuffers || !isCreatedBufferRecycled(buf)) {
      deleteBuffer(buf);
      it = m_created_buffers.erase(it);
    }
    else {
      it++;
    }
  }

  // clean recycled buffers lists if needed
  if (deleteRecycledBuffers) {
    ForeachBufferRecycleType([&](BufferRecycleType type) {
      RecycleData *rdata = m_recycle[type];
      rdata->buffers.clear();
    });
  }
}

TmpBuffer *BufferRecycler::createTmpBuffer(bool is_host_recyclable)
{
  auto buf = new TmpBuffer();
  buf->elem_chs = 0;
  buf->width = 0;
  buf->height = 0;
  buf->is_host_recyclable = is_host_recyclable;
  buf->host.buffer = nullptr;
  buf->host.brow_bytes = 0;
  buf->host.bwidth = 0;
  buf->host.bheight = 0;
  buf->host.belem_chs = 0;
  buf->host.state = HostMemoryState::NONE;
  buf->device.buffer = nullptr;
  buf->device.state = DeviceMemoryState::NONE;
  buf->device.bwidth = 0;
  buf->device.bheight = 0;
  buf->device.belem_chs = 0;
  buf->orig_host.buffer = nullptr;
  buf->orig_host.state = HostMemoryState::NONE;
  buf->orig_host.brow_bytes = 0;
  buf->orig_host.bwidth = 0;
  buf->orig_host.bheight = 0;
  buf->orig_host.belem_chs = 0;
  BLI_assert(!m_execution_id.empty());
  buf->execution_id = m_execution_id;
  buf->n_give_recycles = 0;
  buf->n_take_recycles = 0;
  return buf;
}

TmpBuffer *BufferRecycler::createTmpBuffer(
    bool is_host_recyclable, float *host_buffer, int width, int height, int n_channels)
{
  auto buf = createTmpBuffer(is_host_recyclable);
  buf->elem_chs = n_channels;
  buf->width = width;
  buf->height = height;
  if (host_buffer) {
    buf->host.buffer = host_buffer;
    buf->host.brow_bytes = BufferUtil::calcBufferRowBytes(width, n_channels);
    buf->host.bwidth = width;
    buf->host.bheight = height;
    buf->host.belem_chs = n_channels;
    buf->host.state = host_buffer == nullptr ? HostMemoryState::NONE : HostMemoryState::FILLED;
  }
  return buf;
}

/* returns whether there has been work enqueued to device*/
bool BufferRecycler::takeRecycle(
    BufferRecycleType type, TmpBuffer *dst, int width, int height, int elem_chs)
{
  // by default set requested params. May be changed later if recycle found
  dst->width = width;
  dst->height = height;
  dst->elem_chs = elem_chs;

  bool work_enqueued = false;
  bool recycle_found = recycleFindAndSet(type, dst, width, height, elem_chs);
  if (!recycle_found && type == BufferRecycleType::DEVICE_HOST_ALLOC) {
    // try to find a mapped buffer and unmap it
    recycle_found = recycleFindAndSet(
        BufferRecycleType::DEVICE_HOST_MAPPED, dst, width, height, elem_chs);
    if (recycle_found) {
      BufferUtil::deviceUnmapFromHostEnqueue(dst);
      work_enqueued = true;
    }
  }
  else if (!recycle_found && type == BufferRecycleType::DEVICE_HOST_MAPPED) {
    // try to find a unmapped buffer with host alloc and map it
    recycle_found = recycleFindAndSet(
        BufferRecycleType::DEVICE_HOST_ALLOC, dst, width, height, elem_chs);
    if (recycle_found) {
      BufferUtil::deviceMapToHostEnqueue(dst, MemoryAccess::READ_WRITE);
      work_enqueued = true;
    }
  }

  // alloc and set a new one if there is no recyclable buffer
  if (!recycle_found) {
    switch (type) {
      case BufferRecycleType::HOST_CLEAR:
        BufferUtil::hostAlloc(dst, width, height, elem_chs);
        break;
      case BufferRecycleType::DEVICE_CLEAR:
        BufferUtil::deviceAlloc(dst, MemoryAccess::READ_WRITE, width, height, elem_chs, false);
        break;
      case BufferRecycleType::DEVICE_HOST_ALLOC:
        BufferUtil::deviceAlloc(dst, MemoryAccess::READ_WRITE, width, height, elem_chs, true);
        break;
      case BufferRecycleType::DEVICE_HOST_MAPPED:
        BufferUtil::deviceAlloc(dst, MemoryAccess::READ_WRITE, width, height, elem_chs, true);
        BufferUtil::deviceMapToHostEnqueue(dst, MemoryAccess::READ_WRITE);
        work_enqueued = true;
        break;
      default:
        BLI_assert(!"Non implemented BufferRecycleType");
    }
    m_created_buffers.insert(dst);
  }

  BLI_assert(!m_execution_id.empty());
  dst->execution_id = m_execution_id;

  // TODO delete this
  // assert all recycled buffers are in created buffers list
  ForeachBufferRecycleType([&](BufferRecycleType type) {
    RecycleData *rdata = m_recycle[type];
    for (auto recycled : rdata->buffers) {
      checkRecycledBufferCreated(recycled);
    }
  });
  // End of TODO
  return work_enqueued;
}

/* returns whether it could find a reuse buffer or not */
bool BufferRecycler::recycleFindAndSet(
    BufferRecycleType type, TmpBuffer *dst, int width, int height, int elem_chs)
{
  size_t min_buffer_bytes = BufferUtil::calcBufferBytes(width, height, elem_chs);
  using RecycleData = BufferRecycler::RecycleData;
  RecycleData *rdata = m_recycle[type];

  TmpBuffer *candidate = nullptr;
  float candi_area_scale = FLT_MAX;

  int idx = 0;
  std::unordered_set<TmpBuffer *>::iterator found_it = rdata->buffers.end();
  std::unordered_set<TmpBuffer *>::iterator it = rdata->buffers.begin();
  while (it != rdata->buffers.end()) {
    auto tmp = (*it);
    if (type == BufferRecycleType::HOST_CLEAR) {
      auto tmp_bytes = BufferUtil::calcBufferBytes(
          tmp->host.bwidth, tmp->host.bheight, tmp->host.belem_chs);
      if (tmp_bytes >= min_buffer_bytes) {
        float scale = (float)tmp_bytes / (float)min_buffer_bytes;
        if (scale < candi_area_scale && scale < MAX_BUFFER_SCALE_DIFF_FOR_REUSE) {
          found_it = it;
          if (scale <= 1.0f) {
            break;
          }
        }
      }
    }
    else {
      // device memory handling (OpenCL) don't accept recycling a buffer and using it with a
      // different belem_chs
      if (tmp->device.bwidth >= width && tmp->device.bheight >= height &&
          tmp->device.belem_chs == elem_chs) {
        float scale = RectUtil::area_scale_diff(tmp->width, tmp->height, width, height);
        if (scale < candi_area_scale && scale < MAX_BUFFER_SCALE_DIFF_FOR_REUSE) {
          found_it = it;
          if (scale <= 1.0f) {
            break;
          }
        }
      }
    }

    idx++;
    it++;
  }
  if (found_it != rdata->buffers.end()) {
    candidate = *found_it;
    rdata->buffers.erase(found_it);
  }

  if (candidate) {
    switch (type) {
      case BufferRecycleType::HOST_CLEAR:
        dst->host = std::move(candidate->host);
        BLI_assert(candidate->host.state == HostMemoryState::CLEARED);
        BLI_assert(candidate->device.state == DeviceMemoryState::NONE);
        break;
      case BufferRecycleType::DEVICE_HOST_ALLOC:
        dst->device = std::move(candidate->device);
        BLI_assert(candidate->device.has_map_alloc);
        BLI_assert(candidate->device.state == DeviceMemoryState::CLEARED);
        BLI_assert(candidate->host.state == HostMemoryState::NONE);
        break;
      case BufferRecycleType::DEVICE_CLEAR:
        dst->device = std::move(candidate->device);
        BLI_assert(candidate->device.state == DeviceMemoryState::CLEARED);
        BLI_assert(candidate->host.state == HostMemoryState::NONE);
        break;
      case BufferRecycleType::DEVICE_HOST_MAPPED:
        dst->host = std::move(candidate->host);
        dst->device = std::move(candidate->device);
        BLI_assert(candidate->host.state == HostMemoryState::MAP_FROM_DEVICE);
        BLI_assert(candidate->device.state == DeviceMemoryState::MAP_TO_HOST);
        break;
      default:
        BLI_assert(!"Non implemented BufferRecycleType");
        break;
    }

    dst->n_take_recycles = candidate->n_take_recycles + 1;
    dst->n_give_recycles = candidate->n_give_recycles;

    m_created_buffers.erase(candidate);
    delete candidate;
    m_created_buffers.insert(dst);

    return true;
  }
  else {
    return false;
  }
}

void BufferRecycler::giveRecycle(TmpBuffer *src)
{
#if defined(DEBUG) || defined(COM_DEBUG)
  checkRecycledBufferCreated(src);
#endif

  if (isCreatedBufferRecycled(src)) {
    return;
  }

  TmpBuffer *host_recycle = nullptr;
  TmpBuffer *device_recycle = nullptr;
  TmpBuffer *device_alloc_recycle = nullptr;
  TmpBuffer *device_map_recycle = nullptr;

  if (src->device.state == DeviceMemoryState::MAP_TO_HOST) {
    BLI_assert(src->orig_host.state != HostMemoryState::MAP_FROM_DEVICE);
    BLI_assert(src->host.state == HostMemoryState::MAP_FROM_DEVICE);
    if (src->is_host_recyclable && src->orig_host.state != HostMemoryState::NONE) {
      host_recycle = createTmpBuffer(true, nullptr, src->width, src->height, src->elem_chs);
      host_recycle->host = src->orig_host;
      host_recycle->host.state = HostMemoryState::CLEARED;
      BLI_assert(host_recycle->host.buffer != nullptr);
    }
    device_map_recycle = createTmpBuffer(true, nullptr, src->width, src->height, src->elem_chs);
    device_map_recycle->host = src->host;
    device_map_recycle->device = src->device;
  }
  else {
    BLI_assert(src->orig_host.state == HostMemoryState::NONE);
    BLI_assert(src->host.state != HostMemoryState::MAP_FROM_DEVICE);
    BLI_assert(src->device.state != DeviceMemoryState::MAP_TO_HOST);

    if (src->is_host_recyclable) {
      if (src->host.state == HostMemoryState::CLEARED ||
          src->host.state == HostMemoryState::FILLED) {
        host_recycle = createTmpBuffer(true, nullptr, src->width, src->height, src->elem_chs);
        host_recycle->host = src->host;
        host_recycle->host.state = HostMemoryState::CLEARED;
        BLI_assert(host_recycle->host.buffer != nullptr);
      }
    }

    if (src->device.state == DeviceMemoryState::CLEARED ||
        src->device.state == DeviceMemoryState::FILLED) {
      if (src->device.has_map_alloc) {
        device_alloc_recycle = createTmpBuffer(
            true, nullptr, src->width, src->height, src->elem_chs);
        device_alloc_recycle->device = src->device;
        device_alloc_recycle->device.state = DeviceMemoryState::CLEARED;
      }
      else {
        device_recycle = createTmpBuffer(true, nullptr, src->width, src->height, src->elem_chs);
        device_recycle->device = src->device;
        device_recycle->device.state = DeviceMemoryState::CLEARED;
      }
    }
  }

  if (host_recycle) {
    addRecycle(BufferRecycleType::HOST_CLEAR, src, host_recycle);
  }
  if (device_recycle) {
    addRecycle(BufferRecycleType::DEVICE_CLEAR, src, device_recycle);
  }
  if (device_alloc_recycle) {
    addRecycle(BufferRecycleType::DEVICE_HOST_ALLOC, src, device_alloc_recycle);
  }
  if (device_map_recycle) {
    addRecycle(BufferRecycleType::DEVICE_HOST_MAPPED, src, device_map_recycle);
  }

  delete src;
}

void BufferRecycler::addRecycle(BufferRecycleType type, TmpBuffer *original, TmpBuffer *recycled)
{
  original->n_give_recycles++;
  recycled->n_take_recycles = original->n_take_recycles;
  recycled->n_give_recycles = original->n_give_recycles;
  m_created_buffers.erase(original);
  m_created_buffers.insert(recycled);

  auto rdata = m_recycle[type];
  rdata->buffers.insert(recycled);

  BLI_assert(type != BufferRecycleType::HOST_CLEAR ||
             (recycled->host.buffer != nullptr && recycled->host.bwidth > 0 &&
              recycled->host.bheight > 0 && recycled->host.belem_chs > 0));
  BLI_assert(type == BufferRecycleType::HOST_CLEAR ||
             (recycled->device.buffer != nullptr && recycled->device.bwidth > 0 &&
              recycled->device.bheight > 0 && recycled->device.belem_chs > 0));
}