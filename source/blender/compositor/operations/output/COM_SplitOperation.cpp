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

#include "COM_SplitOperation.h"
#include "BKE_image.h"
#include "BLI_listbase.h"
#include "BLI_math_color.h"
#include "BLI_math_vector.h"
#include "BLI_utildefines.h"
#include "MEM_guardedalloc.h"

#include "COM_BufferUtil.h"
#include "COM_PixelsUtil.h"
#include "IMB_imbuf.h"
#include "IMB_imbuf_types.h"
#include <algorithm>

SplitOperation::SplitOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
}

static void write_rect(std::shared_ptr<PixelsRect> src, PixelsRect &dst, rcti copy_rect)
{
  auto src_r = src->toRect(copy_rect);
  auto dst_r = dst.toRect(copy_rect);
  PixelsUtil::copyEqualRects(dst_r, src_r);
}

void SplitOperation::execPixels(ExecutionManager &man)
{
  auto pix1 = getInputOperation(1)->getPixels(this, man);
  auto pix2 = getInputOperation(2)->getPixels(this, man);

  int split_mark = this->m_xSplit ? this->m_splitPercentage * this->getWidth() / 100.0f :
                                    this->m_splitPercentage * this->getHeight() / 100.0f;

  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext &ctx) {
    rcti pix1_r = (rcti)dst, pix2_r = (rcti)dst;
    if (m_xSplit) {
      pix1_r.xmin = std::max(dst.xmin, split_mark);
      pix2_r.xmax = std::min(dst.xmax, split_mark);
    }
    else {
      pix1_r.ymin = std::max(dst.ymin, split_mark);
      pix2_r.ymax = std::min(dst.ymax, split_mark);
    }
    bool has_pix1 = pix1_r.xmax - pix1_r.xmin > 0 && pix1_r.ymax - pix1_r.ymin > 0;
    bool has_pix2 = pix2_r.xmax - pix2_r.xmin > 0 && pix2_r.ymax - pix2_r.ymin > 0;
    if (has_pix1) {
      write_rect(pix1, dst, pix1_r);
    }
    if (has_pix2) {
      write_rect(pix2, dst, pix2_r);
    }
  };
  cpuWriteSeek(man, cpuWrite);
}

// void SplitOperation::determineResolution(int resolution[2], int preferredResolution[2])
//{
//  int tempPreferredResolution[2] = {0, 0};
//  int tempResolution[2];
//
//  this->getInputSocket(0)->determineResolution(tempResolution, tempPreferredResolution);
//  this->setMainInputSocketIndex((tempResolution[0] && tempResolution[1]) ? 0 : 1);
//
//  NodeOperation::determineResolution(resolution, preferredResolution);
//}
