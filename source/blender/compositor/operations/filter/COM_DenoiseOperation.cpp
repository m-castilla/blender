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
 * Copyright 2019, Blender Foundation.
 */

#include "COM_DenoiseOperation.h"
#include "BLI_math.h"
#include "BLI_system.h"
#ifdef WITH_OPENIMAGEDENOISE
#  include "BLI_threads.h"
#  include <OpenImageDenoise/oidn.hpp>
#endif
#include "COM_ExecutionManager.h"
#include "COM_PixelsUtil.h"
#include "COM_Rect.h"
#include <iostream>

DenoiseOperation::DenoiseOperation()
{
  this->addInputSocket(SocketType::COLOR);
  this->addInputSocket(SocketType::VECTOR);
  this->addInputSocket(SocketType::COLOR);
  this->addOutputSocket(SocketType::COLOR);
  this->m_settings = NULL;
}

void DenoiseOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_settings->hdr);
}

void DenoiseOperation::execPixels(ExecutionManager &man)
{
  auto color = this->getInputOperation(0)->getPixels(this, man);
  auto normal = getInputOperation(1)->getPixels(this, man);
  auto albedo = getInputOperation(2)->getPixels(this, man);

  auto cpu_write = [&](PixelsRect &dst, const WriteRectContext &ctx) {
#ifdef WITH_OPENIMAGEDENOISE
    if (!color->is_single_elem && BLI_cpu_support_sse41()) {
      oidn::DeviceRef device = oidn::newDevice();
      device.commit();

      oidn::FilterRef filter = device.newFilter("RT");

      /* OpenImageDenoise currently only supports RGB, that's why we are using oidn::Format::Float3
       */
      PixelsImg color_img = color->pixelsImg();
      filter.setImage("color",
                      color_img.buffer,
                      oidn::Format::Float3,
                      color_img.row_elems,
                      color_img.col_elems,
                      0,
                      color_img.elem_bytes,
                      color_img.brow_bytes);
      if (!normal->is_single_elem) {
        PixelsImg normal_img = normal->pixelsImg();
        filter.setImage("normal",
                        normal_img.buffer,
                        oidn::Format::Float3,
                        normal_img.row_elems,
                        normal_img.col_elems,
                        0,
                        normal_img.elem_bytes,
                        normal_img.brow_bytes);
      }
      if (!albedo->is_single_elem) {
        PixelsImg albedo_img = normal->pixelsImg();
        filter.setImage("albedo",
                        albedo_img.buffer,
                        oidn::Format::Float3,
                        albedo_img.row_elems,
                        albedo_img.col_elems,
                        0,
                        albedo_img.elem_bytes,
                        albedo_img.brow_bytes);
      }
      PixelsImg dst_img = dst.pixelsImg();
      filter.setImage("output",
                      dst_img.buffer,
                      oidn::Format::Float3,
                      dst_img.row_elems,
                      dst_img.col_elems,
                      0,
                      dst_img.elem_bytes,
                      dst_img.brow_bytes);

      BLI_assert(m_settings);
      if (m_settings) {
        filter.set("hdr", m_settings->hdr);
        filter.set("srgb", false);
      }

      filter.commit();

      /* OpenImageDenoise executes multithreadedly internally. */
      filter.execute();

      /* copy the alpha channel, OpenImageDenoise currently only supports RGB */
      PixelsUtil::copyEqualRectsChannel(dst, 3, *color, 3);
      return;
    }
#endif

    /* If built without OIDN or running on an unsupported CPU, just pass through. */
    UNUSED_VARS(normal, albedo, m_settings);
    PixelsUtil::copyEqualRects(dst, *color);
  };
  cpuWriteSeek(man, cpu_write);
}