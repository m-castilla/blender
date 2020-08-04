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

#ifndef __COM_BUFFERUTIL_H__
#define __COM_BUFFERUTIL_H__

#include "COM_Buffer.h"
#include "COM_defines.h"
#include <memory>

struct rcti;
struct TmpBuffer;
class ExecutionManager;

namespace BufferUtil {
inline bool hasBuffer(BufferType buf_type)
{
  return buf_type != BufferType::NO_BUFFER_WITH_WRITE &&
         buf_type != BufferType::NO_BUFFER_NO_WRITE;
}

std::unique_ptr<TmpBuffer> createUnmanagedTmpBuffer(int n_channels,
                                                    float *host_buffer,
                                                    int host_width,
                                                    int host_height,
                                                    bool is_host_buffer_filled);
void deviceAlloc(TmpBuffer *dst,
                 MemoryAccess device_access,
                 int width,
                 int height,
                 int elem_chs,
                 bool alloc_host);
void deviceFree(TmpBuffer *dst);
float *hostAlloc(int width, int height, int elem_chs);
void hostAlloc(TmpBuffer *dst, int width, int height, int elem_chs);
void hostFree(float *buffer);
void hostFree(TmpBuffer *dst);
void origHostFree(TmpBuffer *dst);
void deleteCacheBuffer(CacheBuffer *buffer);

void deviceMapToHostEnqueue(TmpBuffer *buf, MemoryAccess host_access);
void deviceUnmapFromHostEnqueue(TmpBuffer *buffer);
void deviceToHostCopyEnqueue(TmpBuffer *buf, MemoryAccess host_access);
void hostToDeviceCopyEnqueue(TmpBuffer *buf, MemoryAccess device_access);

}  // namespace BufferUtil

#endif
