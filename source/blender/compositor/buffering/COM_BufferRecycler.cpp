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

const float MAX_AREA_SCALE_FOR_REUSE = 2.0f;
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
  for (auto created_buf : m_created_buffers) {
    BLI_assert(isCreatedBufferRecycled(created_buf));
  }
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

TmpBuffer *BufferRecycler::createTmpBuffer(int n_channels, bool is_host_recyclable)
{
  return createTmpBuffer(n_channels, is_host_recyclable, nullptr, 0, 0);
}

TmpBuffer *BufferRecycler::createTmpBuffer(
    int n_channels, bool is_host_recyclable, float *host_buffer, int host_width, int host_height)
{
  auto buf = new TmpBuffer();
  buf->elem_chs = n_channels;
  buf->is_host_recyclable = is_host_recyclable;
  buf->host.buffer = host_buffer;
  buf->host.width = host_width;
  buf->host.height = host_height;
  buf->host.row_bytes = (size_t)host_width * n_channels * sizeof(float);
  buf->host.state = host_buffer == nullptr ? HostMemoryState::NONE : HostMemoryState::FILLED;
  buf->device.buffer = nullptr;
  buf->device.state = DeviceMemoryState::NONE;
  buf->orig_host.state = HostMemoryState::NONE;
  BLI_assert(!m_execution_id.empty());
  buf->execution_id = m_execution_id;
  buf->n_give_recycles = 0;
  buf->n_take_recycles = 0;
  return buf;
}

/* returns whether there has been work enqueued to device*/
bool BufferRecycler::takeRecycle(
    BufferRecycleType type, TmpBuffer *dst, int width, int height, int elem_chs)
{
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

  return work_enqueued;
}

/* returns whether it could find a reuse buffer or not */
bool BufferRecycler::recycleFindAndSet(
    BufferRecycleType type, TmpBuffer *dst, int width, int height, int elem_chs)
{
  using RecycleData = BufferRecycler::RecycleData;
  RecycleData *rdata = m_recycle[type];

  TmpBuffer *candidate = nullptr;
  float candi_area_scale = MAX_AREA_SCALE_FOR_REUSE;

  int idx = 0;
  std::unordered_set<TmpBuffer *>::iterator found_it = rdata->buffers.end();
  std::unordered_set<TmpBuffer *>::iterator it = rdata->buffers.begin();
  while (it != rdata->buffers.end()) {
    auto tmp = (*it);
    int buf_w = type == BufferRecycleType::HOST_CLEAR ? tmp->host.width : tmp->device.width;
    int buf_h = type == BufferRecycleType::HOST_CLEAR ? tmp->host.height : tmp->device.height;
    if (buf_w >= width && buf_h >= height && tmp->elem_chs == elem_chs) {
      float scale = RectUtil::area_scale_diff(buf_w, buf_h, width, height);
      if (scale < candi_area_scale) {
        found_it = it;
        if (scale <= 1.0f) {
          break;
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
      host_recycle = createTmpBuffer(src->elem_chs, true);
      host_recycle->host = std::move(src->orig_host);
      src->orig_host.buffer = nullptr;
      src->orig_host.state = HostMemoryState::NONE;
      host_recycle->host.state = HostMemoryState::CLEARED;
    }
    device_map_recycle = src;
  }
  else {
    BLI_assert(src->orig_host.state == HostMemoryState::NONE);
    BLI_assert(src->host.state != HostMemoryState::MAP_FROM_DEVICE);
    BLI_assert(src->device.state != DeviceMemoryState::MAP_TO_HOST);

    if (src->is_host_recyclable) {
      if (src->host.state == HostMemoryState::CLEARED ||
          src->host.state == HostMemoryState::FILLED) {
        host_recycle = src->device.state == DeviceMemoryState::NONE ?
                           src :
                           createTmpBuffer(src->elem_chs, true);
        host_recycle->host.state = HostMemoryState::CLEARED;
      }
    }
    else {
      src->host.buffer = nullptr;
      src->host.state = HostMemoryState::NONE;
    }

    if (src->device.state == DeviceMemoryState::CLEARED ||
        src->device.state == DeviceMemoryState::FILLED) {
      if (src->device.has_map_alloc) {
        device_alloc_recycle = src;
        device_alloc_recycle->device.state = DeviceMemoryState::CLEARED;
      }
      else {
        device_recycle = src;
        device_recycle->device.state = DeviceMemoryState::CLEARED;
      }
      src->host.buffer = nullptr;
      src->host.state = HostMemoryState::NONE;
    }
  }

  TmpBuffer *recycled = nullptr;
  if (host_recycle) {
    addRecycle(BufferRecycleType::HOST_CLEAR, host_recycle);
    recycled = host_recycle;
  }
  if (device_recycle) {
    addRecycle(BufferRecycleType::DEVICE_CLEAR, device_recycle);
    recycled = device_recycle;
  }
  if (device_alloc_recycle) {
    addRecycle(BufferRecycleType::DEVICE_HOST_ALLOC, device_alloc_recycle);
    recycled = device_alloc_recycle;
  }
  if (device_map_recycle) {
    addRecycle(BufferRecycleType::DEVICE_HOST_MAPPED, device_map_recycle);
    recycled = device_map_recycle;
  }

  if (recycled) {
    recycled->n_take_recycles = src->n_take_recycles;
    recycled->n_give_recycles = src->n_give_recycles + 1;
  }
}

void BufferRecycler::addRecycle(BufferRecycleType type, TmpBuffer *buf)
{
  auto rdata = m_recycle[type];
  rdata->buffers.insert(buf);
}