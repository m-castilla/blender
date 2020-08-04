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

#ifndef __COM_TONEMAPOPERATION_H__
#define __COM_TONEMAPOPERATION_H__
#include "COM_NodeOperation.h"
#include "DNA_node_types.h"
#include <mutex>

/**
 * \brief temporarily storage during execution of Tonemap
 * \ingroup operation
 */
typedef struct AvgLogLum {
  float al;
  float auto_key;
  float lav;
  float cav[4];
  float igm;
} AvgLogLum;

typedef struct SumLogLum {
  float lav_sum;
  float l_sum;
  float cav_sum[4];
  float maxl, minl;
  int n_rects;
} SumLogLum;

/**
 * \brief base class of tonemap, implementing the simple tonemap
 * \ingroup operation
 */
class TonemapOperation : public NodeOperation {
 protected:
  /**
   * \brief settings of the Tonemap
   */
  NodeTonemap *m_tone;

  /**
   * \brief temporarily cache of the execution storage
   */
  AvgLogLum *m_avg;

  SumLogLum *m_sum;

  std::mutex m_mutex;
  std::mutex m_cv_mutex;
  std::condition_variable m_cv;

 public:
  TonemapOperation();
  virtual void initExecution() override;
  virtual void deinitExecution() override;

  int getNPasses() const
  {
    return 2;
  }
  void setData(NodeTonemap *data)
  {
    this->m_tone = data;
  }

 protected:
  void calcAverage(std::shared_ptr<PixelsRect> color,
                   PixelsRect &dst,
                   const WriteRectContext &ctx);
  bool canCompute() const override
  {
    return false;
  }
  virtual void hashParams();
  virtual void execPixels(ExecutionManager &man) override;
};

/**
 * \brief class of tonemap, implementing the photoreceptor tonemap
 * most parts have already been done in TonemapOperation
 * \ingroup operation
 */
class PhotoreceptorTonemapOperation : public TonemapOperation {
 protected:
  virtual void execPixels(ExecutionManager &man) override;
};

#endif
