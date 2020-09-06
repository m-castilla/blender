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

#include "COM_GlareBaseOperation.h"

GlareBaseOperation::GlareBaseOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
  this->m_settings = nullptr;
}

void GlareBaseOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_settings->angle_ofs);
  hashParam(m_settings->colmod);
  hashParam(m_settings->fade);
  hashParam(m_settings->iter);
  hashParam(m_settings->mix);
  hashParam(m_settings->quality);
  hashParam(m_settings->size);
  hashParam(m_settings->star_45);
  hashParam(m_settings->streaks);
  hashParam(m_settings->threshold);
  hashParam(m_settings->type);
}

void GlareBaseOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);

  auto cpu_write = [&](PixelsRect &dst, const WriteRectContext &) {
    this->generateGlare(dst, *color, this->m_settings, man);
  };
  cpuWriteSeek(man, cpu_write);
}
