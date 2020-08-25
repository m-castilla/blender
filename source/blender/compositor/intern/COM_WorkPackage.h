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

#include "COM_NodeOperation.h"
#include "MEM_guardedalloc.h"
#include <atomic>
#include <functional>
#include <memory>

class PixelsRect;

/**
 * \brief contains data about work that can be scheduled
 * \see WorkScheduler
 */
struct WriteRectContext;
class WorkPackage {
 private:
  ExecutionManager &m_man;
  std::shared_ptr<PixelsRect> m_write_rect;
  std::function<void(PixelsRect &, const WriteRectContext &)> &m_cpu_write_func;
  std::atomic<bool> m_finished;
  WriteRectContext m_write_ctx;

 public:
  WorkPackage(ExecutionManager &man,
              std::shared_ptr<PixelsRect> write_rect,
              std::function<void(PixelsRect &, const WriteRectContext &)> &cpu_write_func);
  ~WorkPackage();
  void setWriteContext(WriteRectContext ctx);
  void exec();
  bool hasFinished()
  {
    return m_finished;
  }
  void reset()
  {
    m_finished = false;
  }

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:WorkPackage")
#endif
};
