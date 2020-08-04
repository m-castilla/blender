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

#ifndef __COM_RECTUTIL_H__
#define __COM_RECTUTIL_H__
#include "BLI_rect.h"
#include <float.h>
#include <vector>
struct rcti;
namespace RectUtil {
typedef struct RectSplit {
  size_t n_rects;
  size_t row_length;
  size_t col_length;
  size_t rects_w;
  size_t rects_h;
  size_t last_rect_in_row_w;
  size_t last_rect_in_col_h;
  bool has_w_equal_rects;
  bool has_h_equal_rects;
  bool has_equal_rects;
} RectSplit;
typedef struct DesiredRectSplits {
  RectSplit desired_split;
  RectSplit top_split;
  RectSplit right_split;
} DesiredRectSplits;

DesiredRectSplits splitRect(size_t rect_w,
                            size_t rect_h,
                            size_t desired_split_size,
                            size_t max_split_size,
                            size_t max_split_w,
                            size_t max_split_h);
RectSplit splitRect(
    size_t rect_w, size_t rect_h, size_t max_split_size, size_t max_split_w, size_t max_split_h);

// Divides the img rect vertically only to avoid buffer img pitch in the rects.  Will divide
// n_rects with same size and if there is height left, it will divide the height left by n_rects
// again and so on until no height is left. Specially useful for executing img writes in parellel
// in a number of threads (n_rects) trying to achieve maximum performance
std::vector<rcti> splitImgRectInEqualRects(int n_rects, int img_width, int img_height);

inline bool includes(const rcti &outside, const rcti &inside)
{
  return outside.xmin <= inside.xmin && outside.xmax >= inside.xmax &&
         outside.ymin <= inside.ymin && outside.ymax >= inside.ymax;
}

inline float area_scale_diff(int outside_w, int outside_h, int inside_w, int inside_h)
{
  int inside_area = inside_w * inside_h;
  int outside_area = outside_w * outside_h;
  if (inside_area == 0 && outside_area == 0) {
    return 0.0f;
  }
  else {
    return inside_area == 0 ? FLT_MAX : (outside_w * outside_h) / inside_area;
  }
}

inline float area_scale_diff(const rcti &outside, const rcti &inside)
{
  return area_scale_diff(BLI_rcti_size_x(&outside),
                         BLI_rcti_size_y(&outside),
                         BLI_rcti_size_x(&inside),
                         BLI_rcti_size_y(&inside));
}

inline static bool collide(const rcti &r1, const rcti &r2)
{
  return r1.xmin < r2.xmax && r1.xmax > r2.xmin && r1.ymin < r2.ymax && r1.ymax > r2.ymin;
}

/**
 * Compares two rects returning which is before the other in coordinates but if they collide
 * they are considered to be in the same coordinates. This is used for detecting rects
 * collisions using binary search with lower_bound and upper_bound. */
inline static bool nonCollideCompare(const rcti &r1, const rcti &r2)
{
  return !collide(r1, r2) && (r1.ymin < r2.ymin || (r1.ymin == r2.ymin && r1.xmin < r2.xmin));
}

};  // namespace RectUtil

#endif
