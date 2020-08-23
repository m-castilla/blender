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

#include "COM_TranslateOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_PixelsUtil.h"
#include "COM_kernel_cpu.h"

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel translateOp(CCL_WRITE(dst),
                       CCL_READ(color),
                       CCL_READ(x_input),
                       CCL_READ(y_input),
                       int color_width,
                       int color_height,
                       BOOL relative,
                       BOOL wrap_x,
                       BOOL wrap_y)

{
  READ_DECL(color);
  READ_DECL(x_input);
  READ_DECL(y_input);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  COPY_COORDS(x_input, dst_coords);
  COPY_COORDS(y_input, dst_coords);

  READ_IMG1(x_input, x_input_pix);
  READ_IMG1(y_input, y_input_pix);

  int2 read_coords;
  if (relative) {
    read_coords.x = dst_coords.x - x_input_pix.x * color_width;
    read_coords.y = dst_coords.y - y_input_pix.x * color_height;
  }
  else {
    read_coords.x = dst_coords.x - x_input_pix.x;
    read_coords.y = dst_coords.y - y_input_pix.x;
  }

  BOOL clip = FALSE;
  if (wrap_x) {
    read_coords.x = wrapf(read_coords.x, color_width, 0);
  }
  else {
    if (read_coords.x < 0 || read_coords.x >= color_width) {
      clip = TRUE;
    }
  }
  if (wrap_y) {
    read_coords.y = wrapf(read_coords.y, color_height, 0);
  }
  else {
    if (read_coords.y < 0 || read_coords.y >= color_height) {
      clip = TRUE;
    }
  }

  if (clip) {
    WRITE_IMG(dst, TRANSPARENT_PIXEL);
  }
  else {
    COPY_COORDS(color, read_coords);
    READ_IMG(color, color_pix);
    WRITE_IMG(dst, color_pix);
  }

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

using namespace std::placeholders;

void TranslateOperation::execPixels(ExecutionManager &man)
{
  bool wrap_x = false;
  bool wrap_y = false;
  switch (m_wrappingType) {
    case CMP_NODE_WRAP_NONE:
      break;
    case CMP_NODE_WRAP_X:
      wrap_x = true;
      break;
    case CMP_NODE_WRAP_Y:
      wrap_y = true;
      break;
    case CMP_NODE_WRAP_XY:
      wrap_x = true;
      wrap_y = true;
      break;
    default:
      break;
  }

  auto color_input = getInputOperation(0)->getPixels(this, man);
  auto x_input = getInputOperation(1)->getPixels(this, man);
  auto y_input = getInputOperation(2)->getPixels(this, man);

  // we need full image input rect because translation can be variant between pixels
  int color_width;
  int color_height;
  if (color_input) {
    color_width = color_input->getWidth();
    color_height = color_input->getHeight();
  }

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::translateOp,
      _1,
      color_input,
      x_input,
      y_input,
      color_width,
      color_height,
      m_relative,
      wrap_x,
      wrap_y);
  computeWriteSeek(man, cpu_write, "translateOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color_input);
    kernel->addReadImgArgs(*x_input);
    kernel->addReadImgArgs(*y_input);
    kernel->addIntArg(color_width);
    kernel->addIntArg(color_height);
    kernel->addBoolArg(m_relative);
    kernel->addBoolArg(wrap_x);
    kernel->addBoolArg(wrap_y);
  });
}

TranslateOperation::TranslateOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::DYNAMIC);
  this->addInputSocket(SocketType::VALUE);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::DYNAMIC);
  this->setMainInputSocketIndex(0);
  this->m_wrappingType = CMP_NODE_WRAP_NONE;
  m_relative = false;
}

void TranslateOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_relative);
  hashParam(m_wrappingType);
}
