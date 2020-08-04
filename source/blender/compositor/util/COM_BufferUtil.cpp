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
#include "BLI_Rect.h"
#include "BLI_assert.h"
#include "COM_ComputeDevice.h"
#include "COM_ExecutionManager.h"
#include "COM_ExecutionSystem.h"
#include "COM_GlobalManager.h"
#include "MEM_guardedalloc.h"
#include <algorithm>
#include <memory>
#include <utility>

namespace BufferUtil {

std::unique_ptr<TmpBuffer> createUnmanagedTmpBuffer(int n_channels,
                                                    float *host_buffer,
                                                    int host_width,
                                                    int host_height,
                                                    bool is_host_buffer_filled)
{
  auto buf = new TmpBuffer();
  buf->elem_chs = n_channels;
  buf->is_host_recyclable = false;
  buf->host.buffer = host_buffer;
  buf->host.width = host_width;
  buf->host.height = host_height;
  buf->host.row_bytes = (size_t)host_width * n_channels * sizeof(float);
  buf->host.state = is_host_buffer_filled ? HostMemoryState::FILLED : HostMemoryState::CLEARED;
  buf->device.buffer = nullptr;
  buf->device.state = DeviceMemoryState::NONE;
  buf->orig_host.state = HostMemoryState::NONE;
  buf->orig_host.buffer = nullptr;
  return std::unique_ptr<TmpBuffer>(buf);
}

void deviceAlloc(TmpBuffer *dst,
                 MemoryAccess device_access,
                 int width,
                 int height,
                 int elem_chs,
                 bool alloc_host)
{
  auto device = GlobalMan->ComputeMan->getSelectedDevice();
  dst->device.buffer = device->memDeviceAlloc(device_access, width, height, elem_chs, alloc_host);
  dst->device.width = width;
  dst->device.height = height;
  dst->elem_chs = elem_chs;
  dst->device.has_map_alloc = alloc_host;
  dst->device.state = DeviceMemoryState::CLEARED;
}

void deviceFree(TmpBuffer *dst)
{
  auto device = GlobalMan->ComputeMan->getSelectedDevice();
  device->memDeviceFree(dst->device.buffer);
  dst->device.buffer = nullptr;
  dst->device.state = DeviceMemoryState::NONE;
}

float *hostAlloc(int width, int height, int elem_chs)
{
  size_t bytes = (size_t)height * width * elem_chs * sizeof(float);
  return (float *)MEM_mallocN(bytes, "COM_BufferUtil::hostAlloc");
}

void hostAlloc(TmpBuffer *dst, int width, int height, int elem_chs)
{
  dst->host.buffer = hostAlloc(width, height, elem_chs);
  dst->host.width = width;
  dst->host.height = height;
  dst->host.row_bytes = (size_t)width * elem_chs * sizeof(float);
  dst->elem_chs = elem_chs;
  dst->host.state = HostMemoryState::CLEARED;
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
  buf->host.buffer = device->memDeviceToHostMapEnqueue(buf->device.buffer,
                                                       host_access,
                                                       buf->device.width,
                                                       buf->device.height,
                                                       buf->elem_chs,
                                                       buf->host.row_bytes);
  buf->host.width = buf->device.width;
  buf->host.height = buf->device.height;
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

void deviceToHostCopyEnqueue(TmpBuffer *buf, MemoryAccess host_access)
{
  BLI_assert(buf->device.state == DeviceMemoryState::FILLED);
  BLI_assert(buf->host.state == HostMemoryState::CLEARED ||
             buf->host.state == HostMemoryState::FILLED);
  auto device = GlobalMan->ComputeMan->getSelectedDevice();
  device->memDeviceToHostCopyEnqueue(buf->host.buffer,
                                     buf->device.buffer,
                                     buf->host.row_bytes,
                                     host_access,
                                     buf->device.width,
                                     buf->device.height,
                                     buf->elem_chs);
  buf->host.state = HostMemoryState::FILLED;
}

void hostToDeviceCopyEnqueue(TmpBuffer *buf, MemoryAccess device_access)
{
  BLI_assert(buf->host.state == HostMemoryState::FILLED);
  BLI_assert(buf->device.state == DeviceMemoryState::CLEARED ||
             buf->device.state == DeviceMemoryState::FILLED);
  auto device = GlobalMan->ComputeMan->getSelectedDevice();
  device->memHostToDeviceCopyEnqueue(buf->device.buffer,
                                     buf->host.buffer,
                                     buf->host.row_bytes,
                                     device_access,
                                     buf->device.width,
                                     buf->device.height,
                                     buf->elem_chs);
  buf->device.state = DeviceMemoryState::FILLED;
}

}  // namespace BufferUtil