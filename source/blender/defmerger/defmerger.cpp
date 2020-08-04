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

#include "../compositor/build_defmerger/COM_DefMerger.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char **argv)
{
  if (argc < 5) {
    printf(
        "Usage: defmerger <macro blocks with TAG to merge> <destination file> <string "
        "with includes to resolve at the start of dst file> <any source dir or "
        "file to merge> ...\n");
    return EXIT_FAILURE;
  }

  const char *tag = argv[1];
  const char *dst_path = argv[2];
  const char *include_str = argv[3];
  DefMerger::defmerge(tag, dst_path, include_str, argc - 4, &argv[4]);
  return EXIT_SUCCESS;
}
