# ***** BEGIN GPL LICENSE BLOCK *****
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software Foundation,
# Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# The Original Code is Copyright (C) 2011, Blender Foundation
# All rights reserved.
# ***** END GPL LICENSE BLOCK *****

set(INC
  .
  intern
  intern/computing
  nodes
  operations
  ../blenkernel
  ../blenlib
  ../blentranslation
  ../depsgraph
  ../imbuf
  ../makesdna
  ../makesrna
  ../nodes
  ../windowmanager
  ../nodes/composite
  ../nodes/intern
  ../render
  ../render/intern
  ../../../extern/clew/include
  ../../../intern/atomic
  ../../../intern/guardedalloc
)

set(INC_SYS

)

set(SRC
  COM_compositor.h
  COM_defines.h

  intern/COM_Buffer.h
  intern/COM_BufferManager.cc
  intern/COM_BufferManager.h
  intern/COM_CPUDevice.cc
  intern/COM_CPUDevice.h
  intern/COM_CompositorContext.cc
  intern/COM_CompositorContext.h
  intern/COM_Converter.cc
  intern/COM_Converter.h
  intern/COM_Enums.cc
  intern/COM_Debug.cc
  intern/COM_Debug.h
  intern/COM_Device.cc
  intern/COM_Device.h
  intern/COM_ExecutionSystem.cc
  intern/COM_ExecutionSystem.h
  intern/COM_MetaData.cc
  intern/COM_MetaData.h
  intern/COM_Node.cc
  intern/COM_Node.h
  intern/COM_NodeConverter.cc
  intern/COM_NodeConverter.h
  intern/COM_NodeGraph.cc
  intern/COM_NodeGraph.h
  intern/COM_NodeOperation.cc
  intern/COM_NodeOperation.h
  intern/COM_NodeOperationBuilder.cc
  intern/COM_NodeOperationBuilder.h
  intern/COM_OutputManager.cc
  intern/COM_OutputManager.h
  intern/COM_PixelUtil.cc
  intern/COM_PixelUtil.h
  intern/COM_SimpleOperation.cc
  intern/COM_SimpleOperation.h
  intern/COM_WorkPackage.cc
  intern/COM_WorkPackage.h
  intern/COM_WorkScheduler.cc
  intern/COM_WorkScheduler.h
  intern/COM_compositor.cc

  operations/COM_QualityStepHelper.cc
  operations/COM_QualityStepHelper.h

    #computing
  intern/computing/COM_compute_types.h
  intern/computing/COM_ComputeDevice.cc
  intern/computing/COM_ComputeDevice.h
  intern/computing/COM_ComputeKernel.cc
  intern/computing/COM_ComputeKernel.h
  intern/computing/COM_ComputeManager.cc
  intern/computing/COM_ComputeManager.h
  intern/computing/COM_ComputeNoneManager.h
  intern/computing/COM_ComputePlatform.cc
  intern/computing/COM_ComputePlatform.h
  intern/computing/opencl/COM_OpenCLDevice.cc
  intern/computing/opencl/COM_OpenCLDevice.h
  intern/computing/opencl/COM_OpenCLKernel.cc
  intern/computing/opencl/COM_OpenCLKernel.h
  intern/computing/opencl/COM_OpenCLManager.cc
  intern/computing/opencl/COM_OpenCLManager.h
  intern/computing/opencl/COM_OpenCLPlatform.cc
  intern/computing/opencl/COM_OpenCLPlatform.h

  # Internal nodes
  nodes/COM_SocketProxyNode.cc
  nodes/COM_SocketProxyNode.h

  # input nodes
  nodes/COM_ImageNode.cc
  nodes/COM_ImageNode.h

  # output nodes
  nodes/COM_ViewerNode.cc
  nodes/COM_ViewerNode.h

  # distort nodes

  # color nodes
  nodes/COM_GammaNode.cc
  nodes/COM_GammaNode.h

  # converter nodes

  # Filter nodes
  nodes/COM_DirectionalBlurNode.cc
  nodes/COM_DirectionalBlurNode.h

  operations/COM_DirectionalBlurOperation.cc
  operations/COM_DirectionalBlurOperation.h

  # Matte nodes
  operations/COM_ImageOperation.cc
  operations/COM_ImageOperation.h


  operations/COM_SocketProxyOperation.cc
  operations/COM_SocketProxyOperation.h

  operations/COM_PreviewOperation.cc
  operations/COM_PreviewOperation.h
  operations/COM_ViewerOperation.cc
  operations/COM_ViewerOperation.h

  operations/COM_GammaOperation.cc
  operations/COM_GammaOperation.h

  operations/COM_SetColorOperation.cc
  operations/COM_SetColorOperation.h
  operations/COM_SetValueOperation.cc
  operations/COM_SetValueOperation.h
  operations/COM_SetVectorOperation.cc
  operations/COM_SetVectorOperation.h

  # Distort operation
  operations/COM_ScaleOperation.cc
  operations/COM_ScaleOperation.h
  operations/COM_TranslateOperation.cc
  operations/COM_TranslateOperation.h

  # Filter operations


  # Convert operations
  operations/COM_ConvertOperation.cc
  operations/COM_ConvertOperation.h

  # Matte operation
)

set(LIB
  bf_blenkernel
  bf_blenlib
  extern_clew
)

list(APPEND INC
  ${CMAKE_CURRENT_BINARY_DIR}/operations
)

data_to_c(
  ${CMAKE_CURRENT_SOURCE_DIR}/operations/COM_OpenCLKernels.cl
  ${CMAKE_CURRENT_BINARY_DIR}/operations/COM_OpenCLKernels.cl.h
  SRC
)

add_definitions(-DCL_USE_DEPRECATED_OPENCL_1_1_APIS)

set(GENSRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/operations)
set(GENSRC ${GENSRC_DIR}/COM_SMAAAreaTexture.h)
add_custom_command(
	OUTPUT ${GENSRC}
	COMMAND ${CMAKE_COMMAND} -E make_directory ${GENSRC_DIR}
	COMMAND "$<TARGET_FILE:smaa_areatex>" ${GENSRC}
	DEPENDS smaa_areatex
)
add_custom_target(smaa_areatex_header
	SOURCES ${GENSRC}
)
list(APPEND SRC
	${GENSRC}
)
unset(GENSRC)
unset(GENSRC_DIR)

if(WITH_INTERNATIONAL)
  add_definitions(-DWITH_INTERNATIONAL)
endif()

if(WITH_OPENIMAGEDENOISE)
  add_definitions(-DWITH_OPENIMAGEDENOISE)
  add_definitions(-DOIDN_STATIC_LIB)
  list(APPEND INC_SYS
    ${OPENIMAGEDENOISE_INCLUDE_DIRS}
    ${TBB_INCLUDE_DIRS}
  )
  list(APPEND LIB
    ${OPENIMAGEDENOISE_LIBRARIES}
    ${TBB_LIBRARIES}
  )
endif()

blender_add_lib(bf_compositor "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID MATCHES "Clang")
  target_compile_options(bf_compositor PRIVATE "-Wsuggest-override")
endif()

add_dependencies(bf_compositor smaa_areatex_header)
