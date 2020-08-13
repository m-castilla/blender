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

/* This implementation has been copied with some modifications from cycles util library */

#include "COM_IncludesResolver.h"
#include <cassert>
#include <map>
#include <set>
#include <stdarg.h>
#include <string>
#include <vector>

#if defined(_WIN32)
#  define DIR_SEP '\\'
#  define DIR_SEP_ALT '/'
#else
#  define DIR_SEP '/'
#endif

using namespace std;
namespace IncludesResolver {

bool string_startswith(const string &s, const char *start)
{
  size_t len = strlen(start);

  if (len > s.size())
    return 0;
  else
    return strncmp(s.c_str(), start, len) == 0;
}

bool string_endswith(const string &s, const char *end)
{
  size_t len = strlen(end);

  if (len > s.size())
    return 0;
  else
    return strncmp(s.c_str() + s.size() - len, end, len) == 0;
}

string string_strip(const string &s)
{
  string result = s;
  result.erase(0, result.find_first_not_of(' '));
  result.erase(result.find_last_not_of(' ') + 1);
  return result;
}

string string_printf(const char *format, ...)
{
  vector<char> str(128, 0);

  while (1) {
    va_list args;
    int result;

    va_start(args, format);
    result = vsnprintf(&str[0], str.size(), format, args);
    va_end(args);

    if (result == -1) {
      /* not enough space or formatting error */
      if (str.size() > 65536) {
        assert(0);
        return string("");
      }

      str.resize(str.size() * 2, 0);
      continue;
    }
    else if (result >= (int)str.size()) {
      /* not enough space */
      str.resize(result + 1, 0);
      continue;
    }

    return string(&str[0]);
  }
}

size_t find_last_slash(const string &path)
{
  for (size_t i = 0; i < path.size(); ++i) {
    size_t index = path.size() - 1 - i;
#ifdef _WIN32
    if (path[index] == DIR_SEP || path[index] == DIR_SEP_ALT)
#else
    if (path[index] == DIR_SEP)
#endif
    {
      return index;
    }
  }
  return string::npos;
}

FILE *path_fopen(const string &path, const string &mode)
{
  return fopen(path.c_str(), mode.c_str());
}

bool path_exists(const string &path)
{
  struct stat st;
  if (stat(path.c_str(), &st) != 0) {
    return 0;
  }
  return st.st_mode != 0;
}

string path_join(const string &dir, const string &file)
{
  if (dir.size() == 0) {
    return file;
  }
  if (file.size() == 0) {
    return dir;
  }
  string result = dir;
#ifndef _WIN32
  if (result[result.size() - 1] != DIR_SEP && file[0] != DIR_SEP)
#else
  if (result[result.size() - 1] != DIR_SEP && result[result.size() - 1] != DIR_SEP_ALT &&
      file[0] != DIR_SEP && file[0] != DIR_SEP_ALT)
#endif
  {
    result += DIR_SEP;
  }
  result += file;
  return result;
}

string path_filename(const string &path)
{
  size_t index = find_last_slash(path);
  if (index != string::npos) {
    /* Corner cases to match boost behavior. */
#ifndef _WIN32
    if (index == 0 && path.size() == 1) {
      return path;
    }
#endif
    if (index == path.size() - 1) {
#ifdef _WIN32
      if (index == 2) {
        return string(1, DIR_SEP);
      }
#endif
      return ".";
    }
    return path.substr(index + 1, path.size() - index - 1);
  }
  return path;
}

string path_dirname(const string &path)
{
  size_t index = find_last_slash(path);
  if (index != string::npos) {
#ifndef _WIN32
    if (index == 0 && path.size() > 1) {
      return string(1, DIR_SEP);
    }
#endif
    return path.substr(0, index);
  }
  return "";
}

struct SourceReplaceState {
  typedef map<string, string> ProcessedMapping;
  /* Base director for all relative include headers. */
  string base;
  /* Result of processed files. */
  ProcessedMapping processed_files;
  /* Set of files which are considered "precompiled" and which are replaced
   * with and empty string on a subsequent occurrence in include statement.
   */
  set<string> precompiled_headers;
};

static string path_source_replace_includes_recursive(const string &source,
                                                     const string &source_filepath,
                                                     SourceReplaceState *state);

static string line_directive(const SourceReplaceState &state, const string &path, const int line)
{
  string unescaped_path = path;
  /* First we make path relative. */
  if (string_startswith(unescaped_path, state.base.c_str())) {
    const string base_file = path_filename(state.base);
    const size_t base_len = state.base.length();
    unescaped_path = base_file +
                     unescaped_path.substr(base_len, unescaped_path.length() - base_len);
  }
  /* Second, we replace all unsafe characters. */
  const size_t length = unescaped_path.length();
  string escaped_path = "";
  for (size_t i = 0; i < length; ++i) {
    const char ch = unescaped_path[i];
    if (strchr("\"\'\?\\", ch) != NULL) {
      escaped_path += "\\";
    }
    escaped_path += ch;
  }
  /* TODO(sergey): Check whether using std::to_string combined with several
   * concatenation operations is any faster.
   */
  return string_printf("#line %d \"%s\"", line, escaped_path.c_str());
}

static int path_stat(const string &path, struct stat *st)
{
  return stat(path.c_str(), st);
}

size_t path_file_size(const string &path)
{
  struct stat st;
  if (path_stat(path, &st) != 0) {
    return -1;
  }
  return st.st_size;
}

bool path_read_binary(const string &path, vector<uint8_t> &binary)
{
  /* read binary file into memory */
  FILE *f = path_fopen(path, "rb");

  if (!f) {
    binary.resize(0);
    return false;
  }

  binary.resize(path_file_size(path));

  if (binary.size() == 0) {
    fclose(f);
    return false;
  }

  if (fread(&binary[0], sizeof(uint8_t), binary.size(), f) != binary.size()) {
    fclose(f);
    return false;
  }

  fclose(f);

  return true;
}

bool path_read_text(const string &path, string &text)
{
  vector<uint8_t> binary;

  if (!path_exists(path) || !path_read_binary(path, binary))
    return false;

  const char *str = (const char *)&binary[0];
  size_t size = binary.size();
  text = string(str, size);

  return true;
}

static string path_source_handle_preprocessor(const string &preprocessor_line,
                                              const string &source_filepath,
                                              const size_t line_number,
                                              SourceReplaceState *state)
{
  string result = preprocessor_line;
  string token = string_strip(preprocessor_line.substr(1, preprocessor_line.size() - 1));
  if (string_startswith(token, "include")) {
    token = string_strip(token.substr(7, token.size() - 7));
    if (token[0] == '"') {
      const size_t n_start = 1;
      const size_t n_end = token.find("\"", n_start);
      const string filename = token.substr(n_start, n_end - n_start);
      const bool is_precompiled = string_endswith(token, "// PRECOMPILED");
      string filepath = path_join(state->base, filename);
      if (!path_exists(filepath)) {
        filepath = path_join(path_dirname(source_filepath), filename);
      }
      if (is_precompiled) {
        state->precompiled_headers.insert(filepath);
      }
      string text;
      if (path_read_text(filepath, text)) {
        text = path_source_replace_includes_recursive(text, filepath, state);
        /* Use line directives for better error messages. */
        result = line_directive(*state, filepath, 1) + "\n" + text + "\n" +
                 line_directive(*state, source_filepath, line_number + 1);
      }
    }
  }
  return result;
}

/* Our own little c preprocessor that replaces #includes with the file
 * contents, to work around issue of OpenCL drivers not supporting
 * include paths with spaces in them.
 */
static string path_source_replace_includes_recursive(const string &source,
                                                     const string &source_filepath,
                                                     SourceReplaceState *state)
{
  /* Try to re-use processed file without spending time on replacing all
   * include directives again.
   */
  SourceReplaceState::ProcessedMapping::iterator replaced_file = state->processed_files.find(
      source_filepath);
  if (replaced_file != state->processed_files.end()) {
    if (state->precompiled_headers.find(source_filepath) != state->precompiled_headers.end()) {
      return "";
    }
    return replaced_file->second;
  }
  /* Perform full file processing. */
  string result = "";
  const size_t source_length = source.length();
  size_t index = 0;
  /* Information about where we are in the source. */
  size_t line_number = 0, column_number = 1;
  /* Currently gathered non-preprocessor token.
   * Store as start/length rather than token itself to avoid overhead of
   * memory re-allocations on each character concatenation.
   */
  size_t token_start = 0, token_length = 0;
  /* Denotes whether we're inside of preprocessor line, together with
   * preprocessor line itself.
   *
   * TODO(sergey): Investigate whether using token start/end position
   * gives measurable speedup.
   */
  bool inside_preprocessor = false;
  string preprocessor_line = "";
  /* Actual loop over the whole source. */
  while (index < source_length) {
    const char ch = source[index];
    if (ch == '\n') {
      if (inside_preprocessor) {
        result += path_source_handle_preprocessor(
            preprocessor_line, source_filepath, line_number, state);
        /* Start gathering net part of the token. */
        token_start = index;
        token_length = 0;
      }
      inside_preprocessor = false;
      preprocessor_line = "";
      column_number = 0;
      ++line_number;
    }
    else if (ch == '#' && column_number == 1 && !inside_preprocessor) {
      /* Append all possible non-preprocessor token to the result. */
      if (token_length != 0) {
        result.append(source, token_start, token_length);
        token_start = index;
        token_length = 0;
      }
      inside_preprocessor = true;
    }
    if (inside_preprocessor) {
      preprocessor_line += ch;
    }
    else {
      ++token_length;
    }
    ++index;
    ++column_number;
  }
  /* Append possible tokens which happened before special events handled
   * above.
   */
  if (token_length != 0) {
    result.append(source, token_start, token_length);
  }
  if (inside_preprocessor) {
    result += path_source_handle_preprocessor(
        preprocessor_line, source_filepath, line_number, state);
  }
  /* Store result for further reuse. */
  state->processed_files[source_filepath] = result;
  return result;
}

string path_source_replace_includes(const string &source,
                                    const string &path,
                                    const string &source_filename)
{
  SourceReplaceState state;
  state.base = path;
  return path_source_replace_includes_recursive(source, path_join(path, source_filename), &state);
}

}  // namespace IncludesResolver
