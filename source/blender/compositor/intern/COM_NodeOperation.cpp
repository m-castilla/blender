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

#include "COM_NodeOperation.h" /* own include */
#include "BLI_assert.h"
#include "COM_BufferUtil.h"
#include "COM_ComputeDevice.h"
#include "COM_ExecutionManager.h"
#include "COM_GlobalManager.h"
#include "COM_MathUtil.h"
#include "COM_Node.h"
#include <typeinfo>

using namespace std::placeholders;

/*******************
 **** NodeOperation ****
 *******************/
NodeOperation::NodeOperation()
    : m_float_hasher(),
      m_key_calculated(false),
      m_key(),
      m_op_hash(0),
      m_exec_pixels_optimized(false),
      base_hash_params_called(false),
      m_single_pixel_mode(false),
      m_single_pixel_x(0),
      m_single_pixel_y(0),
      m_single_pixel()
{
}

NodeOperation::~NodeOperation()
{
}

void NodeOperation::hashParams()
{
  base_hash_params_called = true;
  const std::type_info &typeInfo = typeid(*this);
  m_op_hash = typeInfo.hash_code();
  hashParam(m_width);
  hashParam(m_height);
  for (auto input : m_inputs) {
    auto input_op = input->getLinkedOp();
    if (input_op) {
      MathUtil::hashCombine(m_op_hash, input_op->getKey().op_hash);
    }
  }
}

void NodeOperation::hashFloatData(const float *data, size_t length, int increment)
{
  const float *end = data + length;
  const float *current = data;
  while (current < end) {
    MathUtil::hashCombine(m_op_hash, m_float_hasher(*current));
    current += increment;
  }
}

void NodeOperation::initExecution()
{
  NodeSocketReader::initExecution();
  m_exec_pixels_optimized = false;
}

void NodeOperation::deinitExecution()
{
  NodeSocketReader::deinitExecution();
}

bool NodeOperation::isComputed(ExecutionManager & /*man*/) const
{
  BufferType btype = getBufferType();
  if (btype == BufferType::CUSTOM || btype == BufferType::NO_BUFFER_NO_WRITE ||
      getWriteType() == WriteType::SINGLE_THREAD) {
    return false;
  }
  else {
    return GlobalMan->ComputeMan->canCompute() && this->canCompute();
  }
}

const OpKey &NodeOperation::getKey()
{
  if (!m_key_calculated) {
    // getKey shouldn't been called until operation is initialized.
    // Be sure that you have called base class initExecution() when overriding the method in a
    // subclass
    BLI_assert(m_initialized);

    bool has_persistent_op_key = false;
    if (GlobalMan->CacheMan->isCacheable(this)) {
      auto persistent_result = GlobalMan->CacheMan->checkPersistentOpKey(this);
      has_persistent_op_key = persistent_result.first;
      if (has_persistent_op_key) {
        m_key = persistent_result.second;
        m_op_hash = m_key.op_hash;
      }
    }

    if (!has_persistent_op_key) {
      hashParams();
      m_key.op_width = m_width;
      m_key.op_height = m_height;
      m_key.op_data_type = getOutputDataType();
      m_key.op_type_hash = typeid(*this).hash_code();
      m_key.op_hash = m_op_hash;
    }

    m_key_calculated = true;
  }
  return m_key;
}

void NodeOperation::cpuWriteSeek(
    ExecutionManager &man, std::function<void(PixelsRect &, const WriteRectContext &)> cpu_func)
{
  cpuWriteSeek(man, cpu_func, {});
}

void NodeOperation::cpuWriteSeek(
    ExecutionManager &man,
    std::function<void(PixelsRect &, const WriteRectContext &)> cpu_func,
    std::function<void(PixelsRect &)> after_write_func)
{
  BLI_assert(!this->canCompute());
  computeWriteSeek(man, cpu_func, after_write_func, "", {}, false);
}

void NodeOperation::computeWriteSeek(
    ExecutionManager &man,
    std::function<void(PixelsRect &, const WriteRectContext &)> cpu_func,
    std::string compute_kernel,
    std::function<void(ComputeKernel *)> add_kernel_args_func,
    bool check_call)
{
  computeWriteSeek(man, cpu_func, {}, compute_kernel, add_kernel_args_func, check_call);
}

void NodeOperation::computeWriteSeek(
    ExecutionManager &man,
    std::function<void(PixelsRect &, const WriteRectContext &)> cpu_func,
    std::function<void(PixelsRect &)> after_write_func,
    std::string compute_kernel,
    std::function<void(ComputeKernel *)> add_kernel_args_func,
    bool check_call)
{

  if (check_call) {
    BLI_assert(this->canCompute());
  }
  if (man.canExecPixels()) {
    rcti single_pixel_rect;
    if (m_single_pixel_mode) {
      BLI_rcti_init(&single_pixel_rect,
                    m_single_pixel_x,
                    m_single_pixel_x + 1,
                    m_single_pixel_y,
                    m_single_pixel_y + 1);
    }
    GlobalMan->BufferMan->writeSeek(this,
                                    man,
                                    std::bind(&ExecutionManager::execWriteJob,
                                              &man,
                                              this,
                                              _1,
                                              cpu_func,
                                              after_write_func,
                                              compute_kernel,
                                              add_kernel_args_func,
                                              _2),
                                    m_single_pixel_mode ? &single_pixel_rect : nullptr);
  }
}

// Returns empty when OperationMode is ReadOptimize and when execution has been cancelled.
// To assert that you should have gotten your pixels, Check the condition "man.canExecPixels()"
// after the call. If true, the pixels must have been returned, if not is an implementation error.
std::shared_ptr<PixelsRect> NodeOperation::getPixels(NodeOperation *reader_op,
                                                     ExecutionManager &man)
{
  if (!man.isBreaked()) {
    if (man.getOperationMode() == OperationMode::Optimize) {
      if (!m_exec_pixels_optimized) {
        if (!GlobalMan->CacheMan->hasAnyKindOfCache(this) && !isFinalOperation()) {
          execPixels(man);
        }
        m_exec_pixels_optimized = true;
        man.reportOperationOptimized(this);
      }
      GlobalMan->BufferMan->readOptimize(this, reader_op, man);
    }
    else {
      auto result = GlobalMan->BufferMan->readSeek(this, man);
      if (result.is_written) {
        return result.pixels;
      }
      else {
        if (!isFinalOperation()) {
          execPixels(man);
        }
        result = GlobalMan->BufferMan->readSeek(this, man);
        return result.pixels;
      }
    }
  }

  return std::shared_ptr<PixelsRect>();
}

float *NodeOperation::getSinglePixel(
    NodeOperation *reader_op, ExecutionManager &man, int x, int y, bool check_single_elem)
{
  if (check_single_elem && isSingleElem()) {
    return getSingleElem(man);
  }
  else {
    m_single_pixel_mode = true;
    m_single_pixel_x = x;
    m_single_pixel_y = y;
    auto pixels = getPixels(reader_op, man);
    m_single_pixel_mode = false;
    if (man.canExecPixels() && pixels) {
      auto tmp_buf = pixels->tmp_buffer;
      if (tmp_buf->device.state == DeviceMemoryState::FILLED) {
        auto tmp = BufferUtil::createStdTmpBuffer(m_single_pixel, false, 1, 1, 4);
        tmp->device = tmp_buf->device;
        BufferUtil::deviceToHostCopyEnqueue(tmp.get());
      }
      else {
        auto raw_buf = tmp_buf->host.buffer;
        switch (tmp_buf->elem_chs) {
          case 4:
            m_single_pixel[3] = raw_buf[3];
            [[fallthrough]];
          case 3:
            m_single_pixel[2] = raw_buf[2];
            [[fallthrough]];
          case 2:
            m_single_pixel[1] = raw_buf[1];
            [[fallthrough]];
          case 1:
            m_single_pixel[0] = raw_buf[0];
            break;
          default:
            BLI_assert(!"Unsupported number of channels");
            break;
        }
      }
      return m_single_pixel;
    }
    else {
      return nullptr;
    }
  }
}

void NodeOperation::execPixels(ExecutionManager &man)
{
  cpuWriteSeek(man, [&](PixelsRect & /*dst*/, const WriteRectContext & /*ctx*/) {});
}
