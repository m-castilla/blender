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

#ifndef __COM_DENOISEOPERATION_H__
#define __COM_DENOISEOPERATION_H__

#include "COM_NodeOperation.h"
#include "DNA_node_types.h"

class ExecutionManager;
/* This operation is single thread but OpenImageDenoise executes multithreadedly internally. */
class DenoiseOperation : public NodeOperation {
 private:
  NodeDenoise *m_settings;

 public:
  DenoiseOperation();

  void setDenoiseSettings(NodeDenoise *settings)
  {
    this->m_settings = settings;
  }

  virtual WriteType getWriteType() const
  {
    return WriteType::SINGLE_THREAD;
  }

 protected:
  bool canCompute() const override
  {
    return false;
  }
  virtual void hashParams() override;
  virtual void execPixels(ExecutionManager &man) override;
};

#endif /* __COM_DENOISEOPERATION_H__ */
