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

#include "clew.h"
#include <string>
#include <utility>
#include <vector>

#include "COM_MathUtil.h"
#include "COM_NodeSocketReader.h"
#include "COM_Pixels.h"
#include "COM_Rect.h"

class ExecutionManager;
class ComputeKernel;
struct OpKey;

struct WriteRectContext {
  int n_rects;
  int current_pass;
  int n_passes;
};
/**
 * \brief NodeOperation contains calculation logic
 *
 * Subclasses needs to implement the execution method (defined in SocketReader) to implement logic.
 * \ingroup Model
 */
class NodeOperation : public NodeSocketReader {

 private:
  std::hash<float> m_float_hasher;

  bool m_key_calculated;
  OpKey m_key;
  bool m_op_hash_calculated;
  size_t m_op_hash;
  bool m_exec_pixels_optimized;
  bool base_hash_params_called;

 public:
  virtual ~NodeOperation();

  const OpKey &getKey();
  /**
   * \brief Subclases implementing it must call their base method at the end.
   *
   */
  virtual void initExecution() override;

  /**
   * \brief Subclases implementing it must call their base method at the end.
   *
   */
  virtual void deinitExecution() override;

  bool isComputed(ExecutionManager &man) const;

  // Numbers of times that execPixels will be called for each rect. May be more than 1 for
  // operations that need to read serveral times to do calculations before writing.
  virtual int getNPasses() const
  {
    return 1;
  }

  virtual BufferType getBufferType() const
  {
    return BufferType::TEMPORAL;
  }

  virtual WriteType getWriteType() const
  {
    return WriteType::MULTI_THREAD;
  }

  virtual bool isSingleElem() const
  {
    return false;
  }

  virtual float *getSingleElem()
  {
    BLI_assert(!"This operation is not a single element");
    return nullptr;
  }

  /**
   * \brief is this operation the active viewer output
   * user can select an ViewerNode to be active
   * (the result of this node will be drawn on the backdrop).
   * \return [true:false]
   * \see BaseViewerOperation
   */
  virtual bool isActiveViewerOutput() const
  {
    return false;
  }

  virtual bool isViewerOperation() const
  {
    return false;
  }

  /**
   * \brief isOutputOperation determines whether this operation is an output of the ExecutionSystem
   * during rendering or editing.
   *
   * Default behavior if not overridden, this operation will not be evaluated as being an output of
   * the ExecutionSystem.
   *
   * \see ExecutionSystem
   * \group check
   * \param rendering: [true false]
   *  true: rendering
   *  false: editing
   *
   * \return bool the result of this method
   */
  virtual bool isOutputOperation(bool /*rendering*/) const
  {
    return false;
  }
  virtual bool isPreviewOperation() const
  {
    return false;
  }
  virtual bool isFileOutputOperation() const
  {
    return false;
  }
  virtual bool isProxyOperation() const
  {
    return false;
  }

  // Returns empty when OperationMode is Optimize and when execution has cancelled.
  // To assert that you should have gotten your pixels, Check the condition (!isBreaked() and
  // man.getOperationMode()==OperationMode::Exec) after the call. If true, the pixels must have
  // been returned, if false is an implementation error.
  std::shared_ptr<PixelsRect> getPixels(NodeOperation *reader_op, ExecutionManager &man);

 protected:
  NodeOperation();

  virtual bool canCompute() const
  {
    return true;
  }

  /* Should be overriden when writing pixels is needed */
  virtual void execPixels(ExecutionManager &man);

  void cpuWriteSeek(ExecutionManager &man,
                    std::function<void(PixelsRect &, const WriteRectContext &)> cpu_func,
                    std::function<void(PixelsRect &)> after_write_func);
  void cpuWriteSeek(ExecutionManager &man,
                    std::function<void(PixelsRect &, const WriteRectContext &)> cpu_func);

  void computeWriteSeek(ExecutionManager &man,
                        std::function<void(PixelsRect &, const WriteRectContext &)> cpu_func,
                        std::function<void(PixelsRect &)> after_write_func,
                        std::string compute_kernel,
                        std::function<void(ComputeKernel *)> add_kernel_args_func,
                        bool check_call = true);
  void computeWriteSeek(ExecutionManager &man,
                        std::function<void(PixelsRect &, const WriteRectContext &)> cpu_func,
                        std::string compute_kernel,
                        std::function<void(ComputeKernel *)> add_kernel_args_func,
                        bool check_call = true);

  /**
   * Might be overriden by subclasses with buffer type CUSTOM to return a written
   * buffer that will be added to BufferManager so that any other operation gets the already
   * written pixels.
   *
   * \param buffer
   */
  virtual float *getCustomBuffer()
  {
    return nullptr;
  }

  /**
   * \brief Subclasses must override this method for hashing its own operation parameters. They
   * must call its base class hashParams at the start of the method.
   */
  virtual void hashParams();

  template<class T> inline void hashParam(T param)
  {
    BLI_assert(base_hash_params_called);
    MathUtil::hashCombine(m_op_hash, std::hash<T>()(param));
  }

  void hashDataAsParam(const float *data, size_t length, int increment = 1);

  size_t getOpHash();

 private:
  /* allow the DebugInfo class to look at internals for printing info and let BufferManager get
   * the key, needed for buffers initialization*/
  friend class DebugInfo;
  friend class BufferManager;

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:NodeOperation")
#endif
};
