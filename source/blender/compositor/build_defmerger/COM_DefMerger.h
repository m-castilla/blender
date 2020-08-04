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

#ifndef __DEFMERGE_H__
#define __DEFMERGE_H__

#include <string>

namespace DefMerger {

/* Calling defmerge from source code is mainly for debugging purpuses, for release builds consider
 * using the "defmerger" executable in your CMakeLists.txt*/
std::string defmerge(const char *tag,
                     const char *dst_path,
                     const char *include_str,
                     int n_merge_files,
                     const char *merge_paths[]);

}  // namespace DefMerger

#endif
