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
struct ImBuf;
class ExecutionManager;

#if defined(DEBUG) || defined(COM_DEBUG)
#  define ASSERT_VALID_STD_TMP_BUFFER(tmp_buffer, width, height) \
    BLI_assert((width) > 0 && (height) > 0); \
    BLI_assert(BufferUtil::calcStdBufferBytes((width), (height)) == \
               (tmp_buffer)->getMinBufferBytes())
#else
#  define ASSERT_VALID_STD_TMP_BUFFER(tmp_buffer, width, height)
#endif

namespace BufferUtil {

bool isImBufAvailable(ImBuf *im_buf);
inline size_t calcNonStdBufferBytes(int width, int height, int n_buf_chs)
{
  return (size_t)width * height * n_buf_chs * sizeof(float);
}
inline size_t calcNonStdBufferRowBytes(int width, int n_buf_chs)
{
  return (size_t)width * n_buf_chs * sizeof(float);
}
inline size_t calcStdBufferBytes(int width, int height)
{
  return calcNonStdBufferBytes(width, height, COM_NUM_CHANNELS_STD);
}
inline size_t calcStdBufferRowBytes(int width)
{
  return calcNonStdBufferRowBytes(width, COM_NUM_CHANNELS_STD);
}

inline bool hasBuffer(BufferType buf_type)
{
  return buf_type != BufferType::NO_BUFFER_WITH_WRITE &&
         buf_type != BufferType::NO_BUFFER_NO_WRITE;
}

inline bool hasWrite(BufferType buf_type)
{
  return buf_type != BufferType::NO_BUFFER_NO_WRITE && buf_type != BufferType::CUSTOM;
}

// Returned buffer is non recyclable by default, if you want to recycle, use BufferManager recycler
std::unique_ptr<TmpBuffer> createNonStdTmpBuffer(float *host_buffer = nullptr,
                                                 bool is_host_buffer_filled = false,
                                                 int width = 0,
                                                 int height = 0,
                                                 int n_used_channels = 0,
                                                 int n_buffer_channels = 0);
// Returned buffer is non recyclable by default, if you want to recycle, use BufferManager recycler
std::unique_ptr<TmpBuffer> createStdTmpBuffer(float *host_buffer = nullptr,
                                              bool is_host_buffer_filled = false,
                                              int host_width = 0,
                                              int host_height = 0,
                                              int n_used_channels = 0);
void deviceAlloc(
    TmpBuffer *dst, MemoryAccess device_access, int width, int height, bool alloc_host);
void deviceFree(TmpBuffer *dst);
float *hostAlloc(int width, int height, int elem_chs);
float *hostAlloc(size_t bytes);
void hostNonStdAlloc(TmpBuffer *dst, int width, int height, int belem_chs);
void hostStdAlloc(TmpBuffer *dst, int width, int height);
void hostFree(float *buffer);
void hostFree(TmpBuffer *dst);
void origHostFree(TmpBuffer *dst);

void deviceMapToHostEnqueue(TmpBuffer *buf, MemoryAccess host_access);
void deviceUnmapFromHostEnqueue(TmpBuffer *buffer);
void deviceToHostCopyEnqueue(TmpBuffer *buf);
void hostToDeviceCopyEnqueue(TmpBuffer *buf);

}  // namespace BufferUtil

#endif
