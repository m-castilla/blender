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
#pragma once

#include "COM_defines.h"
#include <functional>

// Key used for mapping a persistent cache to already created OpKey caches
typedef struct PersistentKey {
  int n_frame;
  unsigned int node_id;
  int op_width;
  int op_height;
  DataType op_data_type;
  size_t hash;

  bool const operator==(const PersistentKey &o) const
  {
    return op_width == o.op_width && op_height == o.op_height && op_data_type == o.op_data_type &&
           n_frame == o.n_frame && node_id == o.node_id;
  }
  bool const operator!=(const PersistentKey &o) const
  {
    return !operator==(o);
  }
} PersistentKey;
namespace std {
/* OpKey default hash function. Mainly for being used by default in std::unordermap*/
template<> struct hash<PersistentKey> {
  size_t operator()(const PersistentKey &k) const
  {
    return k.hash;
  }
#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:PersistentKey")
#endif
};

}  // namespace std

typedef struct OpKey {
  int op_width;
  int op_height;
  DataType op_data_type;
  size_t op_type_hash;
  size_t op_hash;

  bool const operator==(const OpKey &o) const
  {
    return op_width == o.op_width && op_height == o.op_height && op_data_type == o.op_data_type &&
           op_type_hash == o.op_type_hash && op_hash == o.op_hash;
  }
  bool const operator!=(const OpKey &o) const
  {
    return !operator==(o);
  }
} OpKey;

namespace std {
/* OpKey default hash function. Mainly for being used by default in std::unordermap*/
template<> struct hash<OpKey> {
  size_t operator()(const OpKey &k) const
  {
    return k.op_hash;
  }
#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:OpKey")
#endif
};
}  // namespace std
