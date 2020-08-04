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

#pragma once

#include "COM_NodeOperation.h"
#include <mutex>

/**
 * \brief base class of CalculateMean, implementing the simple CalculateMean
 * \ingroup operation
 */
class CalculateMeanOperation : public NodeOperation {
 protected:
  int m_setting;
  float m_sum;
  int m_n_pixels;
  float m_result;
  bool m_calculated;
  std::mutex mutex;

 public:
  CalculateMeanOperation(bool add_sockets = true);

  void setSetting(int setting)
  {
    this->m_setting = setting;
  }
  int getSetting()
  {
    return m_setting;
  }
  bool canCompute() const override
  {
    return false;
  }
  BufferType getBufferType() const override
  {
    return BufferType::NO_BUFFER_WITH_WRITE;
  }
  bool isSingleElem() const override
  {
    return true;
  }
  virtual float *getSingleElem() override
  {
    if (!m_calculated) {
      m_result = m_n_pixels > 0 ? m_sum / m_n_pixels : 0;
      m_calculated = true;
    }
    return &m_result;
  }

 protected:
  void hashParams() override;
  virtual void execPixels(ExecutionManager &man) override;
};
