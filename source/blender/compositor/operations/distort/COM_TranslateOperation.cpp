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
#include "COM_GlobalManager.h"
#include "COM_PixelsUtil.h"
#include "COM_kernel_cpu.h"

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

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel translateOp(CCL_WRITE(dst),
                       CCL_READ(img),
                       int translate_x,
                       int translate_y,
                       int last_x,
                       int last_y,
                       BOOL wrap_x,
                       BOOL wrap_y)

{
  READ_DECL(img);
  WRITE_DECL(dst);
  int2 read_coords;

  CPU_LOOP_START(dst);

  read_coords.x = dst_coords.x - translate_x;
  read_coords.y = dst_coords.y - translate_y;

  if (wrap_x) {
    read_coords.x = wrapf(read_coords.x, last_x, 0);
  }
  if (wrap_y) {
    read_coords.y = wrapf(read_coords.y, last_y, 0);
  }

  if (read_coords.x > last_x || read_coords.x < 0 || read_coords.y > last_y || read_coords.y < 0) {
    WRITE_IMG(dst, TRANSPARENT_PIXEL);
  }
  else {
    COPY_COORDS(img, read_coords);
    READ_IMG(img, img_pix);
    WRITE_IMG(dst, img_pix);
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

  auto img_input = getInputOperation(0)->getPixels(this, man);
  auto x_pix = getInputOperation(1)->getSinglePixel(this, man, 0, 0);
  auto y_pix = getInputOperation(2)->getSinglePixel(this, man, 0, 0);

  int color_width = 0;
  int color_height = 0;
  if (img_input) {
    color_width = img_input->getWidth();
    color_height = img_input->getHeight();
  }

  int translate_x = 0, translate_y = 0;
  if (x_pix) {
    translate_x = m_relative ? x_pix[0] * color_width : x_pix[0];
  }
  if (y_pix) {
    translate_y = m_relative ? y_pix[0] * color_height : y_pix[0];
  }
  int last_x = color_width - 1;
  int last_y = color_height - 1;
  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::translateOp, _1, img_input, translate_x, translate_y, last_x, last_y, wrap_x, wrap_y);
  computeWriteSeek(man, cpu_write, "translateOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*img_input);
    kernel->addIntArg(translate_x);
    kernel->addIntArg(translate_y);
    kernel->addIntArg(last_x);
    kernel->addIntArg(last_y);
    kernel->addBoolArg(wrap_x);
    kernel->addBoolArg(wrap_y);
  });
}
