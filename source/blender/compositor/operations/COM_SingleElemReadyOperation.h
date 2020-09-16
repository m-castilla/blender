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
#include <unordered_set>
/// <summary>
/// Any operation inheriting this class will be a single elem operation when all it's input
/// operations are single elem or set as single pixel (the operation will write only the first
/// pixel and will be used for all the pixels). You should be sure that the algorithm is appropiate
/// for this behaviour. If the operation has no input operations or is itself always a single elem
/// operation, you should implement the singleElem methods instead of inheriting this class
/// </summary>
class SingleElemReadyOperation : NodeOperation {
  std::unordered_set<int> m_single_pixel_inputs;

 public:
  bool isSingleElem() const override;
  float *getSingleElem(ExecutionManager &man) override;

  void setInputAsSinglePixel(int input_operation_idx)
  {
    m_single_pixel_inputs.insert(input_operation_idx);
  }
};
