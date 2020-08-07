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

#include "COM_ReadsOptimizer.h"
#include "BLI_assert.h"
#include "COM_ExecutionManager.h"
#include "COM_NodeOperation.h"
#include "COM_RectUtil.h"

/* Pass arguments into this contructor*/
ReadsOptimizer::ReadsOptimizer()
    : m_reads(nullptr), m_after_optimize_setup(false), m_all_readers_reads_gotten(false)
{
}

template<class T> static void freeReadsList(T reads_list)
{
  for (auto read : reads_list) {
    if (read->readers_reads != nullptr) {
      for (auto reader : *read->readers_reads) {
        delete reader.second;
      }
      read->readers_reads->clear();
      delete read->readers_reads;
    }
    delete read;
  }
}

ReadsOptimizer::~ReadsOptimizer()
{
  if (m_reads != nullptr) {
    if (m_reads->readers_reads != nullptr) {
      for (auto reader : *m_reads->readers_reads) {
        delete reader.second;
      }
      m_reads->readers_reads->clear();
      delete m_reads->readers_reads;
    }
    delete m_reads;
  }
}

static OpReads *newOpReads(NodeOperation *op)
{
  return new OpReads{op, 0, 0, 0, 0, false, nullptr};
}

/* By optimize we mean register reads on a operation pixels. Operations are written only once in
 * a full rect buffer. Once written this single buffer can be used for all the reads.*/
void ReadsOptimizer::optimize(NodeOperation *op, NodeOperation *reader_op, ExecutionManager &man)
{
  BLI_assert(man.getOperationMode() == OperationMode::Optimize);
  if (!m_reads) {
    m_reads = newOpReads(op);
  }

  if (reader_op) {
    bool is_reader_computed = reader_op->isComputed(man);
    if (is_reader_computed) {
      m_reads->total_compute_reads++;
    }
    else {
      m_reads->total_cpu_reads++;
    }

    const OpKey &reader_key = reader_op->getKey();
    if (m_reads->readers_reads == nullptr) {
      m_reads->readers_reads = new std::unordered_map<OpKey, ReaderReads *>();
    }

    auto readers_reads = m_reads->readers_reads;
    auto found_reader = readers_reads->find(reader_key);
    ReaderReads *reader_reads = nullptr;
    if (found_reader == readers_reads->end()) {
      reader_reads = new ReaderReads{0, 0, nullptr};
      readers_reads->insert({reader_key, reader_reads});
    }
    else {
      reader_reads = found_reader->second;
    }
    reader_reads->reads = m_reads;

    if (is_reader_computed) {
      reader_reads->n_compute_reads++;
    }
    else {
      reader_reads->n_cpu_reads++;
    }
  }
}

std::vector<std::pair<OpKey, ReaderReads *>> ReadsOptimizer::peepAllReadersReads(
    ExecutionManager &man)
{
  if (!m_all_readers_reads_gotten) {
    m_all_readers_reads_gotten = true;

    std::vector<std::pair<OpKey, ReaderReads *>> result;

    if (m_reads->readers_reads != nullptr) {
      const auto &readers_reads = *m_reads->readers_reads;
      for (const std::pair<OpKey, ReaderReads *> &entry : readers_reads) {
        result.push_back(entry);
      }
    }

    return result;
  }

  BLI_assert(!"This method should be called only once");
  return {};
}

OpReads *ReadsOptimizer::peepReads(ExecutionManager &man)
{
  return m_reads;
}
