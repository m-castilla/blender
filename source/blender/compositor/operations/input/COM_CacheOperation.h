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
#include "COM_defines.h"

/**
 * \brief Compositor output operation
 */
class CacheOperation : public NodeOperation {
 private:
  bool m_persistent;

 public:
  CacheOperation();

  bool isPersistent()
  {
    return m_persistent;
  }
  void setIsPersistent(bool is_persistent)
  {
    m_persistent = is_persistent;
  }
  bool isFinalOperation() override;

 private:
  size_t toBytes(int mbs)
  {
    return (size_t)mbs * 1024 * 1024;
  }

 protected:
  bool canCompute() const override
  {
    return false;
  }
  void hashParams() override;
  void execPixels(ExecutionManager &man) override;
};
