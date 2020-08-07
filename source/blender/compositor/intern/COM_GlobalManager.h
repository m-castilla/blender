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

#ifndef __COM_GLOBALMANAGER_H__
#define __COM_GLOBALMANAGER_H__

#include "COM_BufferManager.h"
#include "COM_ComputeManager.h"
#include "COM_ViewCacheManager.h"
#include <memory>

class GlobalManager;
class NodeOperation;
extern std::unique_ptr<GlobalManager> GlobalMan;
class GlobalManager {
 public:
  BufferManager *BufferMan;
  ComputeManager *ComputeMan;
  ViewCacheManager *ViewCacheMan;

  void initialize(const CompositorContext &ctx);
  void deinitialize(const CompositorContext &ctx);
  bool hasAnyKindOfCache(NodeOperation *op);

  GlobalManager();
  ~GlobalManager();

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:GlobalManagers")
#endif
};

#endif /* __COM_GLOBALMANAGER_H__ */
