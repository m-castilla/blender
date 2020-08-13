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

#include "COM_RectUtil.h"
#include "BLI_assert.h"
#include "COM_MathUtil.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace RectUtil {

DesiredRectSplits splitRect(size_t rect_w,
                            size_t rect_h,
                            size_t desired_split_size,
                            size_t max_split_size,
                            size_t max_split_w,
                            size_t max_split_h)
{
  BLI_assert(rect_w > 0);
  BLI_assert(rect_h > 0);
  BLI_assert(max_split_w > 0);
  BLI_assert(max_split_h > 0);
  BLI_assert(desired_split_size > 0);
  BLI_assert(max_split_size > 0);

  if (max_split_size > rect_w * rect_h) {
    max_split_size = rect_w * rect_h;
  }
  max_split_w = std::min(rect_w, max_split_w);
  max_split_h = std::min(rect_h, max_split_h);

  RectSplit d_split = {0, 0, 0, 0, 0, 0, 0, true, true, true};
  RectSplit top_split = {0, 0, 0, 0, 0, 0, 0, true, true, true};
  RectSplit right_split = {0, 0, 0, 0, 0, 0, 0, true, true, true};
  bool has_desired_splits = desired_split_size <= max_split_w * max_split_h &&
                            desired_split_size <= max_split_size;
  if (has_desired_splits) {
    d_split = splitRect(rect_w, rect_h, desired_split_size, max_split_w, max_split_h);
    size_t remaining_w = d_split.last_rect_in_row_w == d_split.rects_w ?
                             0 :
                             d_split.last_rect_in_row_w;
    size_t remaining_h = d_split.last_rect_in_col_h == d_split.rects_h ?
                             0 :
                             d_split.last_rect_in_col_h;

    if (remaining_w > 0) {
      right_split = splitRect(remaining_w,
                              d_split.col_length * d_split.rects_h,
                              max_split_size,
                              max_split_w,
                              max_split_h);
      d_split.row_length--;
      d_split.n_rects -= d_split.col_length;
      d_split.last_rect_in_row_w = d_split.rects_w;
    }
    if (remaining_h > 0) {
      top_split = splitRect(d_split.row_length * d_split.rects_w + remaining_w,
                            remaining_h,
                            max_split_size,
                            max_split_w,
                            max_split_h);
      d_split.col_length--;
      d_split.n_rects -= d_split.row_length;
      d_split.last_rect_in_col_h = d_split.rects_h;
    }
    d_split.has_equal_rects = true;
    d_split.has_w_equal_rects = true;
    d_split.has_h_equal_rects = true;
  }
  else {
    /* Just split it and assign it all to top since they are not desired rects */
    top_split = splitRect(rect_w, rect_h, max_split_size, max_split_w, max_split_h);
  }
  DesiredRectSplits d_splits;
  d_splits.desired_split = d_split;
  d_splits.right_split = right_split;
  d_splits.top_split = top_split;

  BLI_assert(d_splits.desired_split.has_equal_rects);
  BLI_assert(d_splits.desired_split.has_h_equal_rects);
  BLI_assert(d_splits.desired_split.has_w_equal_rects);

  return d_splits;
}

RectSplit splitRect(
    size_t rect_w, size_t rect_h, size_t max_split_size, size_t max_split_w, size_t max_split_h)
{
  BLI_assert(rect_w > 0);
  BLI_assert(rect_h > 0);
  BLI_assert(max_split_w > 0);
  BLI_assert(max_split_h > 0);
  BLI_assert(max_split_size > 0);

  if (max_split_size > rect_w * rect_h) {
    max_split_size = rect_w * rect_h;
  }
  size_t max_w = std::min(rect_w, max_split_w);
  size_t max_h = std::min(rect_h, max_split_h);

  auto m1m2 = MathUtil::findMultiplesOfNumber(max_split_size, max_w, max_h);
  RectSplit rs;
  rs.rects_w = m1m2.first;
  rs.rects_h = m1m2.second;
  rs.row_length = rect_w / rs.rects_w;
  rs.col_length = rect_h / rs.rects_h;
  rs.n_rects = rs.row_length * rs.col_length;
  size_t w_remainder = rect_w % rs.rects_w;
  if (w_remainder > 0) {
    rs.last_rect_in_row_w = w_remainder;
    rs.row_length++;
    rs.n_rects += rs.col_length;
  }
  else {
    rs.last_rect_in_row_w = rs.rects_w;
  }

  size_t h_remainder = rect_h % rs.rects_h;
  if (h_remainder > 0) {
    rs.last_rect_in_col_h = h_remainder;
    rs.col_length++;
    rs.n_rects += rs.row_length;
  }
  else {
    rs.last_rect_in_col_h = rs.rects_h;
  }
  rs.has_w_equal_rects = rs.last_rect_in_row_w == rs.rects_w;
  rs.has_h_equal_rects = rs.last_rect_in_col_h == rs.rects_h;
  rs.has_equal_rects = rs.has_w_equal_rects && rs.has_h_equal_rects;

  return rs;
}

static void addImgRect(
    std::vector<rcti> &rects, int xmin, int width, int ymin, int height, int start_y)
{
  rcti rect;
  BLI_rcti_init(&rect, 0, width, start_y + ymin, start_y + ymin + height);
  BLI_assert(!BLI_rcti_is_empty(&rect));
  rects.push_back(rect);
}

static std::vector<rcti> splitImgRectInEqualRects(int step_n_rects,
                                                  int width,
                                                  int height,
                                                  int start_y)
{
  BLI_assert(step_n_rects > 0);
  BLI_assert(step_n_rects > 0);

  std::vector<rcti> rects;
  bool has_exact_equal_rects = height % step_n_rects == 0;
  int equal_rects_height = height / step_n_rects;
  if (equal_rects_height == 0) {
    addImgRect(rects, 0, width, 0, height, start_y);
  }
  else {
    for (int i = 0; i < step_n_rects; i++) {
      int ymin = i * equal_rects_height;
      addImgRect(rects, 0, width, ymin, equal_rects_height, start_y);
    }
    if (!has_exact_equal_rects) {
      int height_left = height % step_n_rects;
      auto new_rects = splitImgRectInEqualRects(
          step_n_rects, width, height_left, height - height_left);
      rects.insert(rects.end(), new_rects.begin(), new_rects.end());
    }
  }
  return rects;
}

std::vector<rcti> splitImgRectInEqualRects(int step_n_rects, int width, int height)
{
  BLI_assert(step_n_rects > 0);
  return splitImgRectInEqualRects(step_n_rects, width, height, 0);
}

}  // namespace RectUtil