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
#include "COM_Pixels.h"

void PixelInterpolationForeach(std::function<void(PixelInterpolation)> func)
{
  for (int i = 0; i <= static_cast<int>(PixelInterpolation::BILINEAR); i++) {
    func(static_cast<PixelInterpolation>(i));
  }
}

void PixelExtendForeach(std::function<void(PixelExtend)> func)
{
  for (int i = 0; i <= static_cast<int>(PixelExtend::MIRROR); i++) {
    func(static_cast<PixelExtend>(i));
  }
}