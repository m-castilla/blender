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

#ifndef __COM_MATHUTIL_H__
#define __COM_MATHUTIL_H__

#include <cstdint>
#include <stddef.h> /* For size_t */
#include <utility>
namespace MathUtil {

std::pair<uint64_t, uint64_t> findMultiplesOfNumber(uint64_t num,
                                                    uint64_t max_multiple1,
                                                    uint64_t max_multiple2);

inline void hashCombine(size_t &r_current_hash, size_t hash_to_combine)
{
  r_current_hash ^= hash_to_combine + 2654435769u + (r_current_hash << 6) + (r_current_hash >> 2);
}

};  // namespace MathUtil

#endif
