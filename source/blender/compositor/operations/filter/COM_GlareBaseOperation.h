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
#include "DNA_node_types.h"

class GlareBaseOperation : public NodeOperation {
 private:
  /**
   * \brief settings of the glare node.
   */
  NodeGlare *m_settings;

 public:
  void setGlareSettings(NodeGlare *settings)
  {
    this->m_settings = settings;
  }
  virtual WriteType getWriteType() const override
  {
    return WriteType::SINGLE_THREAD;
  }

 protected:
  GlareBaseOperation();

  virtual void generateGlare(PixelsRect &dst,
                             PixelsRect &src,
                             NodeGlare *settings,
                             ExecutionManager &man) = 0;
  bool canCompute() const override
  {
    return false;
  }
  void hashParams() override;
  void execPixels(ExecutionManager &man) override;
};
