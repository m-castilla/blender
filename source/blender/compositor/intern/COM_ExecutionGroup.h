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

#ifndef __COM_EXECUTIONGROUP_H__
#define __COM_EXECUTIONGROUP_H__

#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif

#include "BLI_rect.h"
#include "COM_CompositorContext.h"
#include "COM_Node.h"
#include "COM_NodeOperation.h"
#include <vector>

class ExecutionSystem;
class MemoryProxy;
class ReadBufferOperation;
class Device;
class CompositorContext;

/**
 * \brief Class ExecutionGroup is a group of Operations that are executed as one.
 * This grouping is used to combine Operations that can be executed as one whole when
 * multi-processing.
 * \ingroup Execution
 */
class ExecutionGroup {
 public:
  typedef std::vector<NodeOperation *> Operations;

 private:
  // fields

  ExecutionSystem &m_sys;
  /**
   * \brief list of operations in this ExecutionGroup
   */
  Operations m_operations;

  OperationMode m_operation_mode;

  /**
   * \brief denotes boundary for border compositing
   * \note measured in pixel space
   */
  rcti m_viewerBorder;

 public:
  ExecutionGroup(ExecutionSystem &sys);

  void setOperationMode(OperationMode mode);
  OperationMode getOperationMode() const;

  bool isBreaked() const;
  /**
   * \brief add an operation to this ExecutionGroup
   * \note this method will add input of the operations recursively
   * \note this method can create multiple ExecutionGroup's
   * \param system:
   * \param operation:
   * \return True if the operation was successfully added
   */
  bool addOperation(NodeOperation *operation);

  /**
   * \brief get the output operation of this ExecutionGroup
   * \return NodeOperation *output operation
   */
  NodeOperation *getOutputOperation() const;
  const rcti &getOutputViewerBorder() const
  {
    return m_viewerBorder;
  }
  bool hasOutputViewerBorder();

  const CompositorContext &getContext() const;

  void initExecution();

  void deinitExecution();

  void execute();

  /**
   * \brief set border for viewer operation
   * \note all the coordinates are assumed to be in normalized space
   */
  void setViewerBorder(float xmin, float xmax, float ymin, float ymax);

  void setRenderBorder(float xmin, float xmax, float ymin, float ymax);

  /* allow the DebugInfo class to look at internals */
  friend class DebugInfo;

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:ExecutionGroup")
#endif
};

#endif
