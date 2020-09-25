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
#include <functional>
#include <memory>

#include "COM_Buffer.h"
#include "COM_Pixels.h"
#include "COM_defines.h"

typedef std::function<std::shared_ptr<PixelsRect>(const rcti)> TmpRectBuilder;
struct TmpBuffer;

class PixelsRect : public rcti {
 public:
  TmpBuffer *tmp_buffer;
  bool is_single_elem;
  float *single_elem;
  int single_elem_chs;

  PixelsRect(TmpBuffer *tmp_buffer, int xmin, int xmax, int ymin, int ymax);
  PixelsRect(TmpBuffer *tmp_buffer, const rcti &rect);
  PixelsRect(float single_elem[COM_NUM_CHANNELS_STD],
             int n_used_chs,
             int xmin,
             int xmax,
             int ymin,
             int ymax);
  PixelsRect(float single_elem[COM_NUM_CHANNELS_STD], int n_used_chs, const rcti &rect);

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
  inline int getBufferElemChs()
  {
    return is_single_elem ? COM_NUM_CHANNELS_STD : tmp_buffer->getBufferElemChs();
  }

  /* This methods are only called for host buffers */
  PixelsImg pixelsImg();

  // Duplicates rect including its buffer. Caller is responsible of recycling the duplicated
  // TmpBuffer. If the duplicated PixelsRect is single elem, this
  // single elem will be copied to fill the entire rect and be converted to a full buffer. So this
  // method ensures that the returned buffer is never a single elem buffer.
  // It ensures that it has no added pitch (row_jump) too, because host recycles should not have it
  // If use_std_recycle == false, unused chs will be deleted
  PixelsRect duplicate(bool use_std_recycle = false);
  /* */

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:PixelsRect")
#endif
};

#endif
