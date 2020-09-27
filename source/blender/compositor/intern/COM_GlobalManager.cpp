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

#include "COM_GlobalManager.h"
#include "COM_ComputeNoneManager.h"
#include "opencl/COM_OpenCLManager.h"

std::unique_ptr<GlobalManager> GlobalMan;

GlobalManager::GlobalManager()
    : BufferMan(nullptr), ComputeMan(nullptr), CacheMan(nullptr), m_context(nullptr)
{
}

GlobalManager::~GlobalManager()
{
  if (BufferMan != nullptr) {
    delete BufferMan;
  }
  if (CacheMan != nullptr) {
    delete CacheMan;
  }
  if (ComputeMan != nullptr) {
    delete ComputeMan;
  }
}

void GlobalManager::initialize(const CompositorContext &ctx)
{
  m_context = &ctx;
  if (CacheMan == nullptr) {
    CacheMan = new CacheManager();
  }
  /* initialize compute manager */
  bool use_opencl = (ctx.getbNodeTree()->flag & NTREE_COM_OPENCL) != 0;
  ComputeType compute_type = use_opencl ? ComputeType::OPENCL : ComputeType::NONE;
  if (!ComputeMan || ComputeMan->getComputeType() != compute_type) {
    ComputeManager *new_compute_man = nullptr;
    switch (compute_type) {
      case ComputeType::NONE:
        new_compute_man = (ComputeManager *)new ComputeNoneManager();
        break;
      case ComputeType::OPENCL:
        new_compute_man = (ComputeManager *)new OpenCLManager();
        break;
      default:
        BLI_assert(!"Non implemented compute type in compositor");
        break;
    }

    // completely delete BufferMan to delete all buffers from previous Compute type
    if (BufferMan) {
      delete BufferMan;
    }
    if (ComputeMan) {
      delete ComputeMan;
    }

    ComputeMan = new_compute_man;
    ComputeMan->initialize();
    BufferMan = new BufferManager(*CacheMan);
    BufferMan->initialize(ctx);
    CacheMan->initialize(&ctx, BufferMan->recycler());
  }
  else {
    BufferMan->initialize(ctx);
    CacheMan->initialize(&ctx, BufferMan->recycler());
  }
}

void GlobalManager::deinitialize(const CompositorContext &ctx)
{
  BufferMan->deinitialize(ctx.isBreaked());
  CacheMan->deinitialize(&ctx);
  m_context = nullptr;
}
