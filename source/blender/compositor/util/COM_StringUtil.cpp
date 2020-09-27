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

#include "BLI_assert.h"
#include <string>

#include "COM_StringUtil.h"

namespace StringUtil {
std::vector<std::string> split(const std::string &str, char delimiter)
{
  std::vector<std::string> parts;
  size_t pos = 0;
  std::string token;
  std::string str_mod(str);
  while ((pos = str_mod.find(delimiter)) != std::string::npos) {
    parts.push_back(str_mod.substr(0, pos));
    str_mod.erase(0, pos + 1);
  }
  if (str_mod.size() > 0) {
    parts.push_back(str_mod);
  }

  return parts;
}

}  // namespace StringUtil
