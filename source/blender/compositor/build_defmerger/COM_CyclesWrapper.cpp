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

#include "COM_CyclesWrapper.h"

#define CCL_NAMESPACE ccl
#define CCL_NAMESPACE_BEGIN namespace CCL_NAMESPACE {
#define CCL_NAMESPACE_END }
#include "../../../intern/cycles/util/util_path.h"
#undef CCL_NAMESPACE_BEGIN
#undef CCL_NAMESPACE_END

namespace CyclesWrapper {
std::string path_source_replace_includes(const std::string &source,
                                         const std::string &path,
                                         const std::string &source_filename)
{
  return CCL_NAMESPACE::path_source_replace_includes(source, path, source_filename);
}

}  // namespace CyclesWrapper
