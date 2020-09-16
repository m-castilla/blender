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

#include "COM_BoxMaskOperation.h"
#include "BLI_math.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"
#include "DNA_node_types.h"

using namespace std::placeholders;
BoxMaskOperation::BoxMaskOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::VALUE);
}

void BoxMaskOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_maskType);
  hashParam(m_data->height);
  hashParam(m_data->rotation);
  hashParam(m_data->width);
  hashParam(m_data->x);
  hashParam(m_data->y);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel boxMaskOp(CCL_WRITE(dst),
                     CCL_READ(mask),
                     CCL_READ(value),
                     int mask_type,
                     const float aspect_inverse,
                     const float w_inverse,
                     const float h_inverse,
                     const float mask_middle_x,
                     const float mask_middle_y,
                     const float mask_half_w,
                     const float mask_half_h,
                     const float cosine,
                     const float sine)
{
  READ_DECL(mask);
  READ_DECL(value);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(mask, dst_coords);

  READ_IMG1(mask, mask_pix);

  float rx = dst_coords.x * w_inverse;
  float ry = dst_coords.y * h_inverse;

  const float dy = (ry - mask_middle_y) * aspect_inverse;
  const float dx = rx - mask_middle_x;
  rx = mask_middle_x + (cosine * dx + sine * dy);
  ry = mask_middle_y + (-sine * dx + cosine * dy);

  bool inside = (rx > mask_middle_x - mask_half_w && rx < mask_middle_x + mask_half_w &&
                 ry > mask_middle_y - mask_half_h && ry < mask_middle_y + mask_half_h);
  if (inside) {
    if (mask_type == CCL_MASK_TYPE_ADD) {
      COPY_COORDS(value, dst_coords);
      READ_IMG1(value, value_pix);
      mask_pix.x = fmaxf(mask_pix.x, value_pix.x);
    }
    else if (mask_type == CCL_MASK_TYPE_SUBTRACT) {
      COPY_COORDS(value, dst_coords);
      READ_IMG1(value, value_pix);
      mask_pix.x = mask_pix.x - value_pix.x;
      mask_pix.x = clamp(mask_pix.x, 0.0f, 1.0f);
    }
    else if (mask_type == CCL_MASK_TYPE_MULTIPLY) {
      COPY_COORDS(value, dst_coords);
      READ_IMG1(value, value_pix);
      mask_pix.x *= value_pix.x;
    }
    else {  // CCL_MASK_TYPE_NOT
      if (mask_pix.x > 0.0f) {
        mask_pix.x = 0.0f;
      }
      else {
        COPY_COORDS(value, dst_coords);
        READ_IMG1(value, mask_pix);
      }
    }
  }
  else {
    if (mask_type == CCL_MASK_TYPE_MULTIPLY) {
      mask_pix.x = 0.0f;
    }
    else if (mask_type == CCL_MASK_TYPE_NOT) {
      COPY_COORDS(mask, dst_coords);
      READ_IMG1(mask, mask_pix);
    }
  }

  WRITE_IMG1(dst, mask_pix);

  CPU_LOOP_END;
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void BoxMaskOperation::execPixels(ExecutionManager &man)
{
  auto mask = getInputOperation(0)->getPixels(this, man);
  auto value = getInputOperation(1)->getPixels(this, man);

  const double rad = (double)m_data->rotation;
  const float cosine = cos(rad);
  const float sine = sin(rad);
  const float aspect_ratio = ((float)getWidth()) / getHeight();
  const float aspect_inverse = 1.0f / aspect_ratio;
  const float mask_middle_x = m_data->x;
  const float mask_middle_y = m_data->y;
  const float w_inverse = 1.0f / getWidth();
  const float h_inverse = 1.0f / getHeight();
  const float mask_half_w = m_data->width / 2.0f;
  const float mask_half_h = m_data->height / 2.0f;

  auto cpu_write = std::bind(CCL::boxMaskOp,
                             _1,
                             mask,
                             value,
                             m_maskType,
                             aspect_inverse,
                             w_inverse,
                             h_inverse,
                             mask_middle_x,
                             mask_middle_y,
                             mask_half_w,
                             mask_half_h,
                             cosine,
                             sine);
  computeWriteSeek(man, cpu_write, "boxMaskOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*mask);
    kernel->addReadImgArgs(*value);
    kernel->addIntArg(m_maskType);
    kernel->addFloatArg(aspect_inverse);
    kernel->addFloatArg(w_inverse);
    kernel->addFloatArg(h_inverse);
    kernel->addFloatArg(mask_middle_x);
    kernel->addFloatArg(mask_middle_y);
    kernel->addFloatArg(mask_half_w);
    kernel->addFloatArg(mask_half_h);
    kernel->addFloatArg(cosine);
    kernel->addFloatArg(sine);
  });
}
