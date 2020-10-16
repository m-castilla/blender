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

#include "COM_UiConvert.h"
#include "BLI_assert.h"

namespace UiConvert {
PixelInterpolation pixelInterpolation(short ui_value)
{
  switch (ui_value) {
    case 0:
      return PixelInterpolation::NEAREST;
    case 1:
      return PixelInterpolation::BILINEAR;
    default:
      BLI_assert("Non implemented pixel interpolation");
      return (PixelInterpolation)0;
  }
}
PixelExtend extendMode(short extend_mode)
{
  switch (extend_mode) {
    case 0:
      return PixelExtend::CLIP;
    case 1:
      return PixelExtend::EXTEND;
    case 2:
      return PixelExtend::REPEAT;
    case 3:
      return PixelExtend::MIRROR;
    default:
      BLI_assert("Non implemented pixel extend");
      return (PixelExtend)0;
  }
}

}  // namespace UiConvert