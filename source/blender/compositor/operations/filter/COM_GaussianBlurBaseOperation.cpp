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

#include "COM_GaussianBlurBaseOperation.h"

GaussianBlurBaseOperation::GaussianBlurBaseOperation(SocketType socket_type)
    : BlurBaseOperation(socket_type)
{
  this->m_falloff = -1; /* intentionally invalid, so we can detect uninitialized values */
  this->m_do_subtract = false;
}

void GaussianBlurBaseOperation::hashParams()
{
  BlurBaseOperation::hashParams();
  hashParam(m_falloff);
  hashParam(m_do_subtract);
}
