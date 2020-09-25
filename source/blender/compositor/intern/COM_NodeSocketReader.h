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

#ifndef __COM_NODESOCKETREADER_H__
#define __COM_NODESOCKETREADER_H__

#include "BLI_assert.h"
#include "DNA_node_types.h"
#include <string>
#include <unordered_map>
#include <vector>

#include "COM_defines.h"
#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif

class NodeOperationInput;
class NodeOperationOutput;
class NodeOperation;
class NodeOutput;
class NodeOperationBuilder;

/**
 * \brief NodeOperation contains calculation logic
 *
 * Subclasses needs to implement the execution method (defined in SocketReader) to implement logic.
 * \ingroup Model
 */
class NodeSocketReader {
 public:
  typedef std::vector<NodeOperationInput *> Inputs;
  typedef std::vector<NodeOperationOutput *> Outputs;

 protected:
  bool m_initialized;
  bool m_deinitialized;
  int m_width;
  int m_height;
  Inputs m_inputs;
  Outputs m_outputs;
  unsigned int m_node_id;

  static uint s_order_counter;
  uint m_order;

  /**
   * \brief the index of the input socket that will be used to determine the resolution
   */
  int m_mainInputSocketIndex;

  /**
   * \brief set to truth when resolution for this operation is set
   */
  bool m_isResolutionSet;

  ResolutionType m_resolution_type;

 public:
  virtual ~NodeSocketReader();
  bool isInitialized()
  {
    return m_initialized;
  }
  bool isDeinitialized()
  {
    return m_deinitialized;
  }
  NodeOperationOutput *getOutputSocket(int index) const;
  NodeOperationOutput *getOutputSocket() const
  {
    return getOutputSocket(0);
  }
  DataType getOutputDataType() const;
  virtual int getOutputNUsedChannels() const;
  NodeOperationInput *getInputSocket(int index) const;
  NodeOperationInput *getMainInputSocket() const
  {
    return getInputSocket(m_mainInputSocketIndex);
  }
  bool hasAnyInputConnected() const;

  void setNodeId(unsigned int id)
  {
    m_node_id = id;
  }
  // unique id of the UI node
  unsigned int getNodeId() const
  {
    return m_node_id;
  }

  /**
   * \brief determine the resolution of this node
   * \note this method will not set the resolution, this is the responsibility of the caller
   * \param resolution: the result of this operation
   * \param preferredResolution: the preferable resolution as no resolution could be determined
   * \param setResolution Used internally, when overriding in a subclass just pass this parameter
   * to the base class determineOperation
   */
  virtual ResolutionType determineResolution(int resolution[2],
                                             int preferredResolution[2],
                                             bool setResolution);

  ResolutionType getResolutionType()
  {
    return m_resolution_type;
  }
  /**
   * \brief set the index of the input socket that will determine the resolution of this operation
   * and its output DataType in case the output is a SocketType::Dynamic. By default will be 0
   * (the first input)
   * \param index: the index to set
   */
  void setMainInputSocketIndex(int index);

  /**
   * \brief Subclases implementing it must call their base method at the end.
   * \param will_exec: whether the operation will be executed or not. This is the case when an
   * evaluated operation is behind an evaluated final operation.
   */
  virtual void initExecution();

  /**
   * \brief Subclases implementing it must call their base method at the end.
   *
   */
  virtual void deinitExecution();

  // Only override when the operation has a condition by which there is no need to execute
  // previous operations in the tree. Default is false.
  virtual bool isFinalOperation()
  {
    return false;
  }

  uint getOrder() const
  {
    return m_order;
  }

  inline int getWidth() const
  {
    return this->m_width;
  }
  inline int getHeight() const
  {
    return this->m_height;
  }

  unsigned int getNumberOfInputSockets() const
  {
    return m_inputs.size();
  }
  unsigned int getNumberOfOutputSockets() const
  {
    return m_outputs.size();
  }

  bool isResolutionSet()
  {
    return this->m_isResolutionSet;
  }

  /** Check if this is an input operation
   * An input operation is an operation that only has output sockets and no input sockets
   */
  bool isInputOperation() const
  {
    return m_inputs.empty();
  }

  void getConnectedInputSockets(Inputs *sockets) const;

 protected:
  NodeSocketReader();

  NodeOperationInput *addInputSocket(SocketType socket_type,
                                     InputResizeMode resize_mode = InputResizeMode::DEFAULT);
  NodeOperationOutput *addOutputSocket(SocketType socket_type);

  NodeOperation *getInputOperation(int inputSocketindex) const;

 private:
  void setResolution(int width, int height, ResolutionType res_type = ResolutionType::Fixed);
  /* allow the DebugInfo class to look at internals for printing info*/
  friend class DebugInfo;
  /* Let Converter, NodeOperationOutput and NodeOperationBuilder call setResolution*/
  friend class Converter;
  friend class NodeOperationOutput;
  friend class NodeOperationBuilder;

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:NodeSocketReader");
#endif
};

class NodeOperationInput {
 private:
  NodeOperation *m_operation;

  SocketType m_socket_type;

  /** Resize mode of this socket */
  InputResizeMode m_resizeMode;

  /** Connected output */
  NodeOperationOutput *m_link;
  bool m_has_user_link;

 public:
  NodeOperationInput(NodeOperation *op,
                     SocketType socket_type,
                     InputResizeMode resizeMode = InputResizeMode::DEFAULT);

  NodeOperation *getOperation() const
  {
    return m_operation;
  }
  DataType getDataType() const;
  bool hasDataType() const;
  int getNChannels() const;
  SocketType getSocketType() const
  {
    return m_socket_type;
  }
  void setLink(NodeOperationOutput *link)
  {
    m_link = link;
  }
  NodeOperationOutput *getLink() const
  {
    return m_link;
  }
  bool isConnected() const
  {
    return m_link;
  }
  // Used to distinguish between real user connections and default value added connections
  bool hasUserLink()
  {
    return isConnected() && m_has_user_link;
  }
  void setHasUserLink(bool has_user_link)
  {
    m_has_user_link = has_user_link;
  }

  void setResizeMode(InputResizeMode resizeMode)
  {
    this->m_resizeMode = resizeMode;
  }
  InputResizeMode getResizeMode() const
  {
    return this->m_resizeMode;
  }

  NodeOperation *getLinkedOp();

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:NodeOperationInput")
#endif
};

class NodeOperationOutput {
 private:
  typedef struct ResolutionResult {
    int calc_width;
    int calc_height;
    int set_width;
    int set_height;
    ResolutionType res_type;
  } ResolutionResult;
  NodeOperation *m_operation;
  SocketType m_socket_type;
  bNodeSocket *m_bOutputSocket;

  // already calculated resolutions from preferred resolutions
  std::unordered_map<uint64_t, ResolutionResult> m_set_res_off;
  std::unordered_map<uint64_t, ResolutionResult> m_set_res_on;

 public:
  NodeOperationOutput(NodeOperation *op, SocketType socket_type);

  NodeOperation *getOperation() const
  {
    return m_operation;
  }
  DataType getDataType() const;
  bool hasDataType() const;
  int getNUsedChannels() const;
  SocketType getSocketType() const
  {
    return m_socket_type;
  }

  void setAsNodeOutput(NodeOutput *nodeOutput);

  bool isNodeOutput()
  {
    return m_bOutputSocket;
  }

  /**
   * \brief determine the resolution of this data going through this socket
   * \param resolution: the result of this operation
   * \param preferredResolution: the preferable resolution as no resolution could be determined
   */
  void determineResolution(int resolution[2], int preferredResolution[2], bool setResolution);

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:NodeOperationOutput")
#endif
};

#endif
