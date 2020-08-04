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

#include "COM_MathUtil.h"
#include "BLI_assert.h"
#include <algorithm>

namespace MathUtil {

std::pair<size_t, size_t> findMultiplesOfNumber(size_t num,
                                                size_t max_multiple1,
                                                size_t max_multiple2)
{
  BLI_assert(max_multiple1 * max_multiple2 >= num);

  /* for the sake of clarity let's make multiple 1 to always be less or equal than multiple 2*/
  bool multiples_switched = max_multiple1 > max_multiple2;
  if (multiples_switched) {
    auto tmp = max_multiple1;
    max_multiple1 = max_multiple2;
    max_multiple2 = tmp;
  }

  size_t m1 = (size_t)sqrt(num);
  size_t m2 = m1;
  size_t current_n = m1 * m2;
  while (current_n != num && m1 > max_multiple1 && m2 > max_multiple2) {
    if (num - current_n >= m1) {
      ++m2;
      current_n += m1;
    }
    else {
      --m1;
      current_n -= m2;
    }
  }

  return multiples_switched ? std::make_pair(m2, m1) : std::make_pair(m1, m2);
}

}  // namespace MathUtil