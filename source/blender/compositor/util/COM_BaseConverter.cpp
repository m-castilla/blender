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

#include "COM_BaseConverter.h"
#include "BLI_assert.h"

const char BASE_CHARS[BaseConverter::MAX_BASE] = {
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
    'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
    'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l',
    'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'};

BaseConverter::BaseConverter()
{
  for (int i = 0; i < MAX_BASE; i++) {
    m_char_to_num.emplace(BASE_CHARS[i], i);
  }
}

std::string BaseConverter::convertBase10ToBase(const size_t decimal, const int radix)
{
  BLI_assert(radix >= MIN_BASE && radix <= MAX_BASE);

  size_t quotient = decimal;
  unsigned char place = 0;

  while (0 != quotient) {
    unsigned long value = quotient;
    m_remainder[place] = value % radix;
    quotient = (value - m_remainder[place]) / radix;

    place++;
  }

  m_ss.str(std::string());
  m_ss.clear();
  for (unsigned char index = 1; index <= place; index++) {
    m_ss << BASE_CHARS[m_remainder[place - index]];
  }
  return m_ss.str();
}

// Function to convert a number from given base 'b'
// to decimal
std::pair<bool, size_t> BaseConverter::convertBaseToBase10(const std::string str, const int radix)
{
  auto result = std::make_pair(false, 0);
  size_t len = str.length();
  size_t power = 1;  // Initialize power of base
  size_t num = 0;    // Initialize result
  int i;

  // Decimal equivalent is str[len-1]*1 +
  // str[len-2]*base + str[len-3]*(base^2) + ...
  for (i = len - 1; i >= 0; i--) {
    // A digit in input number must be
    // less than number's base
    auto char_it = m_char_to_num.find(str[i]);
    if (char_it == m_char_to_num.end() || char_it->second >= radix) {
      // invalid character
      return result;
    }
    else {
      size_t char_value = char_it->second;
      num += char_value * power;
      power = power * radix;
    }
  }

  result.first = true;
  result.second = num;
  return result;
}