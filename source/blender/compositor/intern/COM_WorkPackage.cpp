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

#include "COM_WorkPackage.h"
#include "COM_NodeOperation.h"
#include "COM_Rect.h"

WorkPackage::WorkPackage(
    std::shared_ptr<PixelsRect> write_rect,
    std::function<void(PixelsRect &, const WriteRectContext &)> &cpu_write_func)
    : m_write_rect(write_rect), m_cpu_write_func(cpu_write_func), m_finished(false)
{
}
WorkPackage::~WorkPackage()
{
  int i = 0;
}

void WorkPackage::setWriteContext(WriteRectContext ctx)
{
  m_write_ctx = ctx;
}

void WorkPackage::exec()
{
  m_cpu_write_func(*m_write_rect, m_write_ctx);
}