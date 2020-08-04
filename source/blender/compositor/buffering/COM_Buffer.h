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

#ifndef __COM_BUFFER_H__
#define __COM_BUFFER_H__

#include <string>
enum class MemoryAccess { READ, WRITE, READ_WRITE };
enum class HostMemoryState { NONE, CLEARED, MAP_FROM_DEVICE, FILLED };
enum class DeviceMemoryState { NONE, CLEARED, MAP_TO_HOST, FILLED };
typedef struct HostBuffer {
  float *buffer;
  int width;
  int height;
  size_t row_bytes;
  HostMemoryState state;
} HostBuffer;
typedef struct DeviceBuffer {
  void *buffer;
  int width;
  int height;
  bool has_map_alloc;
  DeviceMemoryState state;
} DeviceBuffer;

typedef struct CacheBuffer {
  HostBuffer host;
  int elem_chs;
  long last_use_time;
} CacheBuffer;
typedef struct TmpBuffer {
  DeviceBuffer device;
  HostBuffer host;
  /* used for tmp save when device is mapped into host */
  HostBuffer orig_host;
  /* recyclable buffers can be reused and deleted. If false host_buffer can't be either reused or
   * deleted because is owned externally. But device_buffer can always be reused or deleted */
  bool is_host_recyclable;
  int elem_chs;
  std::string execution_id;

  /* Used mainly for debugging purpuses*/
  int n_give_recycles;
  int n_take_recycles;
  /**/
} TmpBuffer;

#endif
