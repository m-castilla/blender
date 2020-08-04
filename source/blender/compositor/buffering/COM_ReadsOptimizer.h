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
 * Copyright 2020, Blender Foundation.
 */

#ifndef __COM_READS_OPTIMIZER_H__
#define __COM_READS_OPTIMIZER_H__

#include "COM_NodeOperation.h"
#include "COM_Rect.h"
#include "MEM_guardedalloc.h"
#include <list>
#include <unordered_map>
#include <vector>
class NodeOperation;
struct TmpBuffer;
struct rcti;
class ExecutionManager;

struct ReaderReads;
typedef struct OpReads {
  NodeOperation *op;

  int current_compute_reads;
  int current_cpu_reads;
  int total_compute_reads;
  int total_cpu_reads;

  /* used from BufferManager for writing and saving the reads buffer */
  bool is_write_complete;
  TmpBuffer *tmp_buffer;
  /* */

  std::unordered_map<OpKey, ReaderReads *> *readers_reads;
} OpReads;
typedef struct ReaderReads {
  int n_compute_reads;
  int n_cpu_reads;
  OpReads *reads;
} ReaderReads;

class BufferRecycler;
class ReadsOptimizer {
 public:
 private:
  OpReads *m_reads;
  bool m_after_optimize_setup;
  bool m_all_readers_reads_gotten;

 public:
  ReadsOptimizer();
  ~ReadsOptimizer();

  // must be executed within a single thread
  void optimize(NodeOperation *op, NodeOperation *reader_op, ExecutionManager &man);

  /* must be called ony once */
  std::vector<std::pair<OpKey, ReaderReads *>> peepAllReadersReads(ExecutionManager &man);

  OpReads *peepReads(ExecutionManager &man);

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:ReadsOptimizer")
#endif
};

#endif
