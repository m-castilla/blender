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

#ifndef __COM_VIEWCACHEMANAGER_H__
#define __COM_VIEWCACHEMANAGER_H__

#include "COM_Rect.h"
#include <unordered_map>
#include <unordered_set>

class PreviewOperation;
class ViewerOperation;
class NodeOperation;
struct Image;
class ViewCacheManager {
 private:
  typedef struct PreviewCache {
    OpKey op_key;
    unsigned char *buffer;
  } PreviewCache;
  std::unordered_map<unsigned int, PreviewCache *> m_previews;
  std::unordered_map<unsigned int, OpKey> m_viewers;
  std::unordered_set<unsigned int> m_exec_previews;

 public:
  ~ViewCacheManager();
  void initialize();
  void deinitialize(bool isBreaked);

  // Will return NULL if there is no cache or was invalidated
  unsigned char *getPreviewCache(PreviewOperation *op);
  // must be called after finishing writing a preview
  void reportPreviewWrite(PreviewOperation *op, unsigned char *buffer_to_cache);

  bool hasViewCache(NodeOperation *op);

  // check if viewer needs to update the image
  bool viewerNeedsUpdate(ViewerOperation *op);
  // must be called after finishing writing a viewer
  void reportViewerWrite(ViewerOperation *op);

 private:
  void deletePreviewCache(PreviewCache *cache);

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:BufferManager")
#endif
};

#endif
