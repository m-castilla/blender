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

#include "COM_DefMerger.h"
#include "BLI_fileops.h"
#include "BLI_fileops_types.h"
#include "BLI_path_util.h"
#include "BLI_string.h"
#include <algorithm>

#undef CHECK_TYPE
#undef CHECK_TYPE_PAIR
#undef CHECK_TYPE_INLINE

#define CCL_NAMESPACE ccl
#define CCL_NAMESPACE_BEGIN namespace CCL_NAMESPACE {
#define CCL_NAMESPACE_END }
#include "../../../intern/cycles/util/util_path.h"
#undef CCL_NAMESPACE_BEGIN
#undef CCL_NAMESPACE_END

namespace DefMerger {

void mergeFileorDir(const char *path, const char *search_tag, std::string &result);
/* Will return the next line after the start tag if found otherwise NULL*/
const char *search_macro_start_tag(const char *str, const char *tag, const char **r_end_macro);
/* Will return the position of the macro end*/
const char *search_macro_end_tag(const char *str, const char *end_macro, const char *tag);
/* returns next line after the macro if found otherwise NULL*/
const char *rsearch_macro_in_line(const char *full_s1, const char *str, const char *macro);
const char *rstrstr_until_newline(const char *full_s1, const char *from_s1_pos, const char *s2);
const char *strstr_until_newline(const char *s1, const char *s2);
const char *next_line(const char *s1);

std::string defmerge(const char *tag,
                     const char *dst_path,
                     const char *include_str,
                     int n_merge_paths,
                     const char *merge_paths[])
{
  std::string result = std::string(include_str);
  result.append("\n");
  bool is_empty_include = result.empty();
  for (int i = 0; i < n_merge_paths; i++) {
    const char *merge_path = merge_paths[i];
    mergeFileorDir(merge_path, tag, result);
  }

  if (!is_empty_include) {
    char dir_path[FILE_MAX];
    char file_name[FILE_MAX];
    BLI_split_dir_part(dst_path, dir_path, FILE_MAX);
    BLI_split_file_part(dst_path, file_name, FILE_MAX);
    result = CCL_NAMESPACE::path_source_replace_includes(result, dir_path, file_name);
/*Remove carriage returns which outputs file with double carriage on windows. They get probably
added because cycles "path_source_replace_includes" reads source files as binaries instead of text.
So windows new lines are not converted properly from \r\n  to \n on reading, so on writing it would
result in \r\r\n */
#ifdef _WIN32
    result.erase(std::remove(result.begin(), result.end(), '\r'), result.end());
#endif
  }
  if (!result.empty()) {
    FILE *dst_file = fopen(dst_path, "w+");
    fputs(result.c_str(), dst_file);
    fclose(dst_file);
  }
  return result;
}

void mergeFileorDir(const char *path, const char *search_tag, std::string &result)
{
  if (BLI_is_dir(path)) {
    char dir_path[FILE_MAX];
    strcpy(dir_path, path);
    BLI_path_slash_ensure(dir_path);
    struct direntry *dir;
    const uint dir_len = BLI_filelist_dir_contents(dir_path, &dir);
    for (uint i = 0; i < dir_len; i++) {
      auto fpath = dir[i].path;
      /* Don't follow links. */
      const eFileAttributes file_attrs = BLI_file_attributes(fpath);
      if (file_attrs & FILE_ATTR_ANY_LINK) {
        continue;
      }

      char file[FILE_MAX];
      BLI_split_dirfile(fpath, NULL, file, 0, sizeof(file));
      if (!FILENAME_IS_CURRPAR(file)) {
        mergeFileorDir(fpath, search_tag, result);
      }
    }
    BLI_filelist_free(dir, dir_len);
  }
  else {
    auto ext = BLI_path_extension(path);
    if (ext) {
      auto ext_str = std::string(ext);
      std::transform(ext_str.begin(), ext_str.end(), ext_str.begin(), [](unsigned char c) {
        return std::tolower(c);
      });
      if (ext_str == ".c" || ext_str == ".cpp" || ext_str == ".h" || ext_str == ".hpp") {
        size_t file_size;
        const char *file_str = (const char *)BLI_file_read_text_as_mem(path, 0, &file_size);
        const char *macro_start_cur = file_str;
        const char *macro_end_cur;
        const char *macro_end = "";
        do {
          macro_start_cur = search_macro_start_tag(macro_start_cur, search_tag, &macro_end);
          if (macro_start_cur != NULL) {
            macro_end_cur = search_macro_end_tag(macro_start_cur, macro_end, search_tag);
            if (macro_end_cur != NULL) {
              result.append(file_str, macro_start_cur - file_str, macro_end_cur - macro_start_cur);
              macro_start_cur = next_line(macro_end_cur);
            }
          }
        } while (macro_start_cur != NULL);
      }
    }
  }
}

/* Will return the next line after the start tag if found otherwise NULL*/
const char *search_macro_start_tag(const char *str, const char *tag, const char **r_end_macro)
{
  const char *tag_result = strstr(str, tag);

  if (tag_result != NULL) {
    auto result = rsearch_macro_in_line(str, tag_result, "define");
    if (result != NULL) {
      *r_end_macro = "undef";
      return next_line(result);
    }
    result = rsearch_macro_in_line(str, tag_result, "ifdef");
    if (result != NULL) {
      *r_end_macro = "endif";
      return next_line(result);
    }
    result = rstrstr_until_newline(str, tag_result, "ifndef");
    if (result != NULL) {
      *r_end_macro = "endif";
      return next_line(result);
    }
  }

  return NULL;
}

/* Will return the position of the macro end*/
const char *search_macro_end_tag(const char *str, const char *end_macro, const char *tag)
{
  const char *result = NULL;
  if (end_macro == "undef") {
    result = strstr(str, tag);
    return rsearch_macro_in_line(str, result, end_macro);
  }
  else {
    result = strstr(str, end_macro);
    if (result != NULL) {
      result = rstrstr_until_newline(str, result, "#");
    }
  }

  if (result != NULL) {
    return result;
  }
  else {
    return NULL;
  }
}

/* returns next line after the macro if found otherwise NULL*/
const char *rsearch_macro_in_line(const char *full_str,
                                  const char *from_str_pos,
                                  const char *macro)
{
  const char *result = rstrstr_until_newline(full_str, from_str_pos, macro);
  if (result != NULL) {
    result = rstrstr_until_newline(full_str, result, "#");
  }
  return result;
}

const char *rstrstr_until_newline(const char *full_s1, const char *from_s1_pos, const char *s2)
{
  size_t s1len = strlen(from_s1_pos);
  size_t s2len = strlen(s2);
  const char *s;

  if (s2len > s1len) {
    return NULL;
  }
  for (s = from_s1_pos - s2len; s >= full_s1; --s) {
    if (*s == '\n') {
      return NULL;
    }
    if (strncmp(s, s2, s2len) == 0) {
      return s;
    }
  }
  return NULL;
}

const char *strstr_until_newline(const char *s1, const char *s2)
{
  size_t s2len = strlen(s2);
  const char *s = s1;
  char c = *s;
  while (c != '\r' && c != '\n' && c != '\0') {
    if (strncmp(s, s2, s2len) == 0) {
      return s;
    }
    ++s;
    c = *s;
  }
  return NULL;
}

const char *next_line(const char *s1)
{
  const char *s = s1;
  char c = *s;
  while (c != '\n') {
    ++s;
    c = *s;
    if (c == '\0') {
      return NULL;
    }
  }
  return ++s;
}

}  // namespace DefMerger
