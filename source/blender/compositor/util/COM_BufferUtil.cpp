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

#include "COM_BufferUtil.h"
#include "BLI_assert.h"
#include "COM_ComputeDevice.h"
#include "COM_ExecutionManager.h"
#include "COM_ExecutionSystem.h"
#include "COM_GlobalManager.h"
#include "IMB_imbuf_types.h"
#include "MEM_guardedalloc.h"
#include <algorithm>
#include <memory>
#include <utility>

namespace BufferUtil {

bool isImBufAvailable(ImBuf *imbuf)
{
  return imbuf != NULL && (imbuf->rect != NULL || imbuf->rect_float != NULL);
}

std::unique_ptr<TmpBuffer> createNonStdTmpBuffer(float *host_buffer,
                                                 bool is_host_buffer_filled,
                                                 int width,
                                                 int height,
                                                 int n_used_channels,
                                                 int n_buffer_channels)
{
  if (n_buffer_channels == 0) {
    n_buffer_channels = n_used_channels;
  }

  auto buf = new TmpBuffer();
  buf->is_host_recyclable = false;
  buf->elem_chs3 = n_used_channels;
  buf->host.buffer = host_buffer;
  buf->width = width;
  buf->height = height;

  buf->host.brow_bytes = BufferUtil::calcNonStdBufferRowBytes(width, n_buffer_channels);
  buf->host.bheight = height;
  buf->host.bwidth = width;
  buf->host.belem_chs3 = n_buffer_channels;
  if (host_buffer == nullptr) {
    buf->host.state = HostMemoryState::NONE;
  }
  else {
    buf->host.state = is_host_buffer_filled ? HostMemoryState::FILLED : HostMemoryState::CLEARED;
  }

  buf->device.buffer = nullptr;
  buf->device.bwidth = 0;
  buf->device.bheight = 0;
  buf->device.belem_chs3 = 0;
  buf->device.state = DeviceMemoryState::NONE;

  buf->orig_host.buffer = nullptr;
  buf->orig_host.brow_bytes = 0;
  buf->orig_host.bwidth = 0;
  buf->orig_host.bheight = 0;
  buf->orig_host.state = HostMemoryState::NONE;

  buf->execution_id = "";
  buf->n_give_recycles = 0;
  buf->n_take_recycles = 0;

  return std::unique_ptr<TmpBuffer>(buf);
}

std::unique_ptr<TmpBuffer> createStdTmpBuffer(float *host_buffer,
                                              bool is_host_buffer_filled,
                                              int host_width,
                                              int host_height,
                                              int n_used_channels)
{
  return createNonStdTmpBuffer(host_buffer,
                               is_host_buffer_filled,
                               host_width,
                               host_height,
                               n_used_channels,
                               COM_NUM_CHANNELS_STD);
}

void deviceAlloc(
    TmpBuffer *dst, MemoryAccess device_access, int width, int height, bool alloc_host)
{
  auto device = GlobalMan->ComputeMan->getSelectedDevice();
  dst->device.buffer = device->memDeviceAlloc(device_access, width, height, alloc_host);
  dst->device.bwidth = width;
  dst->device.bheight = height;
  dst->device.belem_chs3 = COM_NUM_CHANNELS_STD;
  dst->device.has_map_alloc = alloc_host;
  dst->device.state = DeviceMemoryState::CLEARED;
  ASSERT_VALID_STD_TMP_BUFFER(dst, width, height);
}

void deviceFree(TmpBuffer *dst)
{
  auto device = GlobalMan->ComputeMan->getSelectedDevice();
  device->memDeviceFree(dst->device.buffer);
  dst->device.buffer = nullptr;
  dst->device.state = DeviceMemoryState::NONE;
}

float *hostAlloc(int width, int height, int belem_chs)
{
  size_t bytes = (size_t)height * width * belem_chs * sizeof(float);
  return (float *)MEM_mallocN_aligned(bytes, 16, "COM_BufferUtil::hostNonStdAlloc");
}

void hostNonStdAlloc(TmpBuffer *dst, int width, int height, int belem_chs)
{
  dst->host.buffer = hostAlloc(width, height, belem_chs);
  dst->host.bwidth = width;
  dst->host.bheight = height;
  dst->host.belem_chs3 = belem_chs;
  dst->host.brow_bytes = BufferUtil::calcNonStdBufferRowBytes(width, belem_chs);
  dst->host.buffer_bytes = BufferUtil::calcNonStdBufferBytes(width, height, belem_chs);
  BLI_assert(dst->host.buffer_bytes == dst->host.brow_bytes * dst->host.bheight);
  dst->host.state = HostMemoryState::CLEARED;
}

void hostStdAlloc(TmpBuffer *dst, int width, int height)
{
  hostNonStdAlloc(dst, width, height, COM_NUM_CHANNELS_STD);
  ASSERT_VALID_STD_TMP_BUFFER(dst, width, height);
}

void hostFree(float *buffer)
{
  if (buffer) {
    MEM_freeN(buffer);
  }
}

void hostFree(TmpBuffer *dst)
{
  hostFree(dst->host.buffer);
  dst->host.buffer = nullptr;
  dst->host.state = HostMemoryState::NONE;
}

void origHostFree(TmpBuffer *dst)
{
  hostFree(dst->orig_host.buffer);
  dst->orig_host.buffer = nullptr;
  dst->orig_host.state = HostMemoryState::NONE;
}

void deleteCacheBuffer(CacheBuffer *buffer)
{
  hostFree(buffer->host.buffer);
  delete buffer;
}

void deviceMapToHostEnqueue(TmpBuffer *buf, MemoryAccess host_access)
{
  BLI_assert(buf->device.state == DeviceMemoryState::CLEARED ||
             buf->device.state == DeviceMemoryState::FILLED);
  auto device = GlobalMan->ComputeMan->getSelectedDevice();
  buf->orig_host = std::move(buf->host);
  if (buf->device.state == DeviceMemoryState::CLEARED) {
    host_access = MemoryAccess::WRITE;
  }
  buf->host.bwidth = buf->width;
  buf->host.bheight = buf->height;
  buf->host.belem_chs3 = COM_NUM_CHANNELS_STD;

  // set default brow_bytes, this may be changed later by memDeviceToHostMapEnqueue
  buf->host.brow_bytes = BufferUtil::calcStdBufferRowBytes(buf->width);

  buf->host.buffer = device->memDeviceToHostMapEnqueue(
      buf->device.buffer, host_access, buf->width, buf->height, buf->host.brow_bytes);
  buf->host.state = HostMemoryState::MAP_FROM_DEVICE;
  buf->device.state = DeviceMemoryState::MAP_TO_HOST;
}

void deviceUnmapFromHostEnqueue(TmpBuffer *buf)
{
  BLI_assert(buf->device.state == DeviceMemoryState::MAP_TO_HOST);
  BLI_assert(buf->host.state == HostMemoryState::MAP_FROM_DEVICE);
  auto device = GlobalMan->ComputeMan->getSelectedDevice();

  device->memDeviceToHostUnmapEnqueue(buf->device.buffer, buf->host.buffer);
  buf->host = std::move(buf->orig_host);
  buf->device.state = DeviceMemoryState::FILLED;
}

void deviceToHostCopyEnqueue(TmpBuffer *buf)
{
  BLI_assert(buf->device.state == DeviceMemoryState::FILLED);
  BLI_assert(buf->host.state == HostMemoryState::CLEARED ||
             buf->host.state == HostMemoryState::FILLED);
  auto device = GlobalMan->ComputeMan->getSelectedDevice();
  device->memDeviceToHostCopyEnqueue(
      buf->host.buffer, buf->device.buffer, buf->host.brow_bytes, buf->width, buf->height);
  buf->host.state = HostMemoryState::FILLED;
}

void hostToDeviceCopyEnqueue(TmpBuffer *buf)
{
  BLI_assert(buf->host.state == HostMemoryState::FILLED);
  BLI_assert(buf->device.state == DeviceMemoryState::CLEARED ||
             buf->device.state == DeviceMemoryState::FILLED);
  auto device = GlobalMan->ComputeMan->getSelectedDevice();
  device->memHostToDeviceCopyEnqueue(
      buf->device.buffer, buf->host.buffer, buf->host.brow_bytes, buf->width, buf->height);
  buf->device.state = DeviceMemoryState::FILLED;
}

}  // namespace BufferUtil