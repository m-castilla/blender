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

#ifndef __COM_RECT_H__
#define __COM_RECT_H__

#include "BLI_rect.h"
#include "COM_Buffer.h"
#include "COM_Pixels.h"
#include "COM_defines.h"
#include <functional>

typedef std::function<std::shared_ptr<PixelsRect>(const rcti)> TmpRectBuilder;
struct TmpBuffer;
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
  std::size_t operator()(const OpKey &k) const
  {
    return k.op_hash;
  }
};

}  // namespace std

class PixelsRect : public rcti {
 public:
  TmpBuffer *tmp_buffer;
  bool is_single_elem;
  float *single_elem;
  int single_elem_chs;

  PixelsRect(TmpBuffer *tmp_buffer, int xmin, int xmax, int ymin, int ymax);
  PixelsRect(TmpBuffer *tmp_buffer, const rcti &rect);
  PixelsRect(float *single_elem, int single_elem_chs, int xmin, int xmax, int ymin, int ymax);
  PixelsRect(float *single_elem, int single_elem_chs, const rcti &rect);

  PixelsRect toRect(const rcti &rect);

  inline int getWidth() const
  {
    return xmax - xmin;
  }
  inline int getHeight() const
  {
    return ymax - ymin;
  }
  inline int getElemChs()
  {
    return is_single_elem ? single_elem_chs : tmp_buffer->elem_chs;
  }

  /* This methods are only called for host buffers */
  PixelsImg pixelsImg();

  // Duplicates rect including its buffer. Caller is responsible of recycling the duplicated
  // TmpBuffer. If the duplicated PixelsRect is single elem, this
  // single elem will be copied to fill the entire rect and be converted to a full buffer. So this
  // method ensures that the returned buffer is never a single elem buffer.
  // It ensures that it has no added pitch (row_jump) too, because host recycles should not have it
  PixelsRect duplicate();
  /* */

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:PixelsRect")
#endif
};

#endif
