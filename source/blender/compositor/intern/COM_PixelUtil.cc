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
 * Copyright 2011, Blender Foundation.
 */

#include "COM_PixelUtil.h"
#include <algorithm>

namespace blender::compositor {

using std::max;
using std::min;

void PixelUtil::writePixel(CPUBuffer<float> &buf, int x, int y, const float color[4])
{
  if (x >= buf.rect.xmin && x < buf.rect.xmax && y >= buf.rect.ymin && y < buf.rect.ymax) {
    const int offset = buf.row_jump * y + x * buf.elem_jump;
    memcpy(&buf[offset], color, buf.elem_bytes);
  }
}

void PixelUtil::addPixel(CPUBuffer<float> &buf, int x, int y, const float color[4])
{
  if (x >= buf.rect.xmin && x < buf.rect.xmax && y >= buf.rect.ymin && y < buf.rect.ymax) {
    const int offset = buf.row_jump * (y - buf.rect.ymin) + (x - buf.rect.xmin) * buf.elem_jump;
    float *dst = &buf[offset];
    const float *src = color;
    for (int i = 0; i < buf.elem_chs; i++, dst++, src++) {
      *dst += *src;
    }
  }
}

static void read_ewa_pixel_sampled(void *userdata, int x, int y, float result[4])
{
  const CPUBuffer<float> &buffer = *((const CPUBuffer<float> *)userdata);
  PixelUtil::read(buffer, result, x, y);
}

void PixelUtil::readEWA(const CPUBuffer<float> &buf,
                        float *result,
                        const float uv[2],
                        const float derivatives[2][2])
{
  BLI_assert(buf.elem_chs == COM_data_type_num_channels(DataType::Color));
  float inv_width = 1.0f / (float)buf.width, inv_height = 1.0f / (float)buf.height;
  /* TODO(sergey): Render pipeline uses normalized coordinates and derivatives,
   * but compositor uses pixel space. For now let's just divide the values and
   * switch compositor to normalized space for EWA later.
   */
  float uv_normal[2] = {uv[0] * inv_width, uv[1] * inv_height};
  float du_normal[2] = {derivatives[0][0] * inv_width, derivatives[0][1] * inv_height};
  float dv_normal[2] = {derivatives[1][0] * inv_width, derivatives[1][1] * inv_height};

  BLI_ewa_filter(buf.width,
                 buf.height,
                 false,
                 true,
                 uv_normal,
                 du_normal,
                 dv_normal,
                 read_ewa_pixel_sampled,
                 (CPUBuffer<float> *)&buf,
                 result);
}

}  // namespace blender::compositor
