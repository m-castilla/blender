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

#include "COM_SingleElemReadyOperation.h"
#include "COM_BufferUtil.h"

bool SingleElemReadyOperation::isSingleElem() const
{
  int n_inputs = getNumberOfInputSockets();
  // If no inputs, you should implement single elem methods in your own class
  BLI_assert(n_inputs > 0);
  for (int i = 0; i < n_inputs; i++) {
    if (!getInputOperation(i)->isSingleElem() &&
        m_single_pixel_inputs.find(i) == m_single_pixel_inputs.end()) {
      return false;
    }
  }
  return true;
}

float *SingleElemReadyOperation::getSingleElem(ExecutionManager &man)
{
  BLI_assert(isSingleElem());
  return getSinglePixel(this, man, 0, 0, false);
}
