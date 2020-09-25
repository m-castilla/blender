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

#include "COM_ViewCacheManager.h"
#include "BLI_assert.h"
#include "COM_PreviewOperation.h"
#include "COM_ViewerOperation.h"
#include "DNA_image_types.h"
#include "MEM_guardedalloc.h"

ViewCacheManager::~ViewCacheManager()
{
  for (auto &cache_pair : m_previews) {
    auto cache = cache_pair.second;
    MEM_freeN(cache->buffer);
  }
}

void ViewCacheManager::initialize()
{
  m_exec_previews.clear();
}
void ViewCacheManager::deinitialize(bool isBreaked)
{
  if (!isBreaked) {
    for (auto it = m_previews.begin(); it != m_previews.end();) {
      auto preview_key = it->first;
      if (m_exec_previews.find(preview_key) == m_exec_previews.end()) {
        auto cache = it->second;
        deletePreviewCache(cache);
        it = m_previews.erase(it);
      }
      else {
        it++;
      }
    }
  }
  m_exec_previews.clear();
}

unsigned char *ViewCacheManager::getPreviewCache(PreviewOperation *op)
{
  const OpKey &op_key = op->getKey();
  auto preview_key = op->getPreviewKey();
  m_exec_previews.insert(preview_key);
  auto found_it = m_previews.find(preview_key);
  if (found_it != m_previews.end()) {
    auto cache = found_it->second;
    if (cache->op_key == op_key) {
      return cache->buffer;
    }
    else {
      deletePreviewCache(cache);
      m_previews.erase(found_it);
    }
  }
  return nullptr;
}

void ViewCacheManager::reportPreviewWrite(PreviewOperation *op, unsigned char *buffer_to_cache)
{
  const OpKey &op_key = op->getKey();
  auto preview_key = op->getPreviewKey();
  BLI_assert(m_previews.find(preview_key) == m_previews.end());
  m_previews.insert({preview_key, new PreviewCache{op_key, buffer_to_cache}});
}

bool ViewCacheManager::viewerNeedsUpdate(ViewerOperation *op)
{
  auto image = op->getImage();
  const OpKey &op_key = op->getKey();
  unsigned int img_id = image->id.session_uuid;
  auto found_it = m_viewers.find(img_id);
  if (found_it != m_viewers.end()) {
    auto last_op_key = found_it->second;
    if (last_op_key == op_key) {
      return false;
    }
  }
  return true;
}

bool ViewCacheManager::hasViewCache(NodeOperation *op)
{
  if (typeid(*op) == typeid(PreviewOperation)) {
    return getPreviewCache((PreviewOperation *)op) != nullptr;
  }
  else if (typeid(*op) == typeid(ViewerOperation)) {
    return !viewerNeedsUpdate((ViewerOperation *)op);
  }
  else {
    return false;
  }
}

// must be called after finishing writing a viewer
void ViewCacheManager::reportViewerWrite(ViewerOperation *op)
{
  auto image = op->getImage();
  m_viewers.insert_or_assign(image->id.session_uuid, op->getKey());
}

void ViewCacheManager::deletePreviewCache(PreviewCache *cache)
{
  MEM_freeN(cache->buffer);
  delete cache;
}