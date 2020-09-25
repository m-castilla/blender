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

#pragma once

#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>

class BaseConverter {
 public:
  static const int MAX_BASE = 62;
  static const int MIN_BASE = 10;  // could be 2 if needed, but as of now we avoid it so that
                                   // we don't have to allocate big array for remainders
 private:
  // should be more than enough for MIN_BASE==10
  static const int MAX_BASE_STRING_CHARS = 64;

  std::stringstream m_ss;
  size_t m_remainder[MAX_BASE_STRING_CHARS];
  std::unordered_map<char, size_t> m_char_to_num;

 public:
  BaseConverter();
  std::string convertBase10ToBase(const size_t decimal, const int radix);
  // returns where string has valid characters, and the result if true
  std::pair<bool, size_t> convertBaseToBase10(const std::string str, const int radix);
};
