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

#include "COM_BufferManager.h"
#include "COM_ComputeManager.h"
#include "COM_ExecutionGroup.h"
#include "COM_ExecutionManager.h"
#include "COM_Node.h"
#include "COM_NodeOperation.h"
#include "DNA_color_types.h"
#include "DNA_node_types.h"
#include <memory>
#include <vector>

class ExecutionGroup;
/**
 * \brief the ExecutionSystem contains the whole compositor tree.
 */
class ExecutionSystem {
 private:
 public:
  typedef std::vector<NodeOperation *> Operations;
  typedef std::vector<ExecutionGroup *> Groups;
  bNodeTree *m_bNodeTree;

 private:
  /**
   * \brief the context used during execution
   */
  const CompositorContext &m_context;

  /**
   * \brief vector of operations
   */
  Operations m_operations;

  /**
   * \brief vector of groups
   */
  Groups m_groups;

  const std::unordered_map<OpKey, std::vector<ReaderReads *>> *m_readers_reads;

  struct OpDeps {
    NodeOperation *op;
    int n_deps;
  };
  struct OpDepsSorter {
    inline bool operator()(const OpDeps &o1, const OpDeps &o2)
    {
      return (o1.n_deps < o2.n_deps);
    }
  };
  std::unordered_map<OpKey, OpDeps> m_ops_deps;

 public:
  /**
   * \brief Create a new ExecutionSystem and initialize it with the
   * editingtree.
   *
   * \param editingtree: [bNodeTree *]
   * \param rendering: [true false]
   */
  ExecutionSystem(const CompositorContext &context);

  /**
   * Destructor
   */
  ~ExecutionSystem();

  void set_operations(const Operations &operations, const Groups &groups);

  void initGroups(ExecutionManager &man);
  /**
   * \brief execute this system
   * - initialize the NodeOperation's and ExecutionGroup's
   * - schedule the output ExecutionGroup's based on their priority
   * - deinitialize the ExecutionGroup's and NodeOperation's
   */
  void execute();

  void deinitGroups(ExecutionManager &man);

  int getNOperations() const
  {
    return m_operations.size();
  }

  inline bool isBreaked() const
  {
    return m_context.isBreaked();
  }

  /**
   * \brief get the reference to the compositor context
   */
  const CompositorContext &getContext() const
  {
    return this->m_context;
  }

 private:
  void execGroups(ExecutionManager &man);
  std::vector<OpDeps> getOperationsOrderedByNDepends(ExecutionManager &man);
  int getOperationNDepends(NodeOperation *op);

  /* allow the DebugInfo class to look at internals */
  friend class DebugInfo;

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:ExecutionSystem")
#endif
};
