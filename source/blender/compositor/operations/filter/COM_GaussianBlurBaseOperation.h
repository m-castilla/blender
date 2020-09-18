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

#ifndef __COM_GAUSSIANBLURBASEOPERATION_H__
#define __COM_GAUSSIANBLURBASEOPERATION_H__
#include "COM_BlurBaseOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_GlobalManager.h"
#include "COM_NodeOperation.h"
#include "COM_kernel_cpu.h"
#include <functional>

class GaussianBlurBaseOperation : public BlurBaseOperation {
 protected:
  int m_falloff; /* falloff for distbuf_inv */
  bool m_do_subtract;

 public:
  GaussianBlurBaseOperation(SocketType socket_type);

  /**
   * Set subtract for Dilate/Erode functionality
   */
  void setSubtract(bool subtract)
  {
    this->m_do_subtract = subtract;
  }
  void setFalloff(int falloff)
  {
    this->m_falloff = falloff;
  }

 protected:
  virtual void hashParams() override;

  /**
   * must be called by subclasses
   */
  template<class KernelFunc>
  void execGaussianPixels(ExecutionManager &man,
                          bool is_x,
                          bool is_alpha_blur,
                          KernelFunc kernel_func,
                          std::string kernel_name)
  {
    auto src = getInputOperation(0)->getPixels(this, man);

    int quality_step = QualityStepHelper::getStep();

    float rad;
    int filter_size;
    TmpBuffer *gausstab = nullptr;
    TmpBuffer *distbuf_inv = nullptr;
    bool do_invert = m_do_subtract;
    if (man.canExecPixels()) {
      short data_size = is_x ? m_data.sizex : m_data.sizey;
      rad = CCL::fmaxf(m_size * data_size, 0.0f);
      filter_size = CCL::fminf(ceilf(rad), MAX_GAUSSTAB_RADIUS);
      gausstab = BlurBaseOperation::make_gausstab(rad, filter_size);
      if (is_alpha_blur) {
        distbuf_inv = BlurBaseOperation::make_dist_fac_inverse(rad, filter_size, m_falloff);
      }
    }

    int width = this->getWidth();
    int height = this->getHeight();
    std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
        kernel_func,
        std::placeholders::_1,
        src,
        gausstab ? gausstab->host.buffer : nullptr,
        distbuf_inv ? distbuf_inv->host.buffer : nullptr,
        do_invert,
        filter_size,
        is_x ? width : height,
        quality_step);
    computeWriteSeek(man, cpu_write, kernel_name, [&](ComputeKernel *kernel) {
      kernel->addReadImgArgs(*src);
      kernel->addFloatArrayArg(
          gausstab->host.buffer, gausstab->width * gausstab->height, MemoryAccess::READ);

      if (distbuf_inv) {
        kernel->addFloatArrayArg(distbuf_inv->host.buffer,
                                 distbuf_inv->width * distbuf_inv->height,
                                 MemoryAccess::READ);
      }
      else {
        kernel->addFloatArrayArg(nullptr, 0, MemoryAccess::READ);
      }
      kernel->addBoolArg(do_invert);
      kernel->addIntArg(filter_size);
      kernel->addIntArg(is_x ? width : height);
      kernel->addIntArg(quality_step);
    });

    if (gausstab) {
      GlobalMan->BufferMan->recycler()->giveRecycle(gausstab);
    }
    if (distbuf_inv) {
      GlobalMan->BufferMan->recycler()->giveRecycle(distbuf_inv);
    }
  }
};
#endif
