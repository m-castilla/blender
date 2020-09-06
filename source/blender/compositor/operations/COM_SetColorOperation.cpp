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

#include "COM_SetColorOperation.h"
#include "COM_PixelsUtil.h"
#include <string.h>

SetColorOperation::SetColorOperation() : NodeOperation(), m_color()
{
  this->addOutputSocket(SocketType::COLOR);
}

void SetColorOperation::hashParams()
{
  NodeOperation::hashParams();
  hashDataAsParam(m_color, COM_NUM_CHANNELS_COLOR);
}

void SetColorOperation::setChannels(const float value[COM_NUM_CHANNELS_COLOR])
{
  memcpy(m_color, value, sizeof(float) * COM_NUM_CHANNELS_COLOR);
}