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

get_filename_component(CMP_BASE ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)
#set(CMP_BASE ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(CMP_NODES ${CMP_BASE}/nodes)
set(CMP_OPS ${CMP_BASE}/operations)



set(INC
  ${CMP_BASE}
  ${CMP_BASE}/buffering
  ${CMP_BASE}/caching
  ../build_defmerger
  ${CMP_BASE}/intern
  ${CMP_BASE}/util
  ${CMP_BASE}/computing
  
  ${CMP_NODES}
  ${CMP_NODES}/input
  ${CMP_NODES}/output
  ${CMP_NODES}/color
  ${CMP_NODES}/converter
  ${CMP_NODES}/filter
  ${CMP_NODES}/vector
  ${CMP_NODES}/matte
  ${CMP_NODES}/distort
  ${CMP_NODES}/layout
  ${CMP_NODES}/experimental

  ${CMP_OPS}
  ${CMP_OPS}/input
  ${CMP_OPS}/output
  ${CMP_OPS}/color
  ${CMP_OPS}/converter
  ${CMP_OPS}/filter
  ${CMP_OPS}/vector
  ${CMP_OPS}/matte
  ${CMP_OPS}/distort
  ${CMP_OPS}/experimental

  ${CMP_BASE}/../blenkernel
  ${CMP_BASE}/../blenlib
  ${CMP_BASE}/../blentranslation
  ${CMP_BASE}/../depsgraph
  ${CMP_BASE}/../imbuf
  ${CMP_BASE}/../makesdna
  ${CMP_BASE}/../makesrna
  ${CMP_BASE}/../nodes
  ${CMP_BASE}/../windowmanager
  ${CMP_BASE}/../nodes/composite
  ${CMP_BASE}/../nodes/intern
  ${CMP_BASE}/../render/extern/include
  ${CMP_BASE}/../render/intern/include
  ${CMP_BASE}/../../../extern/clew/include
  ${CMP_BASE}/../../../intern/atomic
  ${CMP_BASE}/../../../intern/cycles
  ${CMP_BASE}/../../../intern/guardedalloc
)

set(INC_SYS
  ${BOOST_INCLUDE_DIR}
)

set(SRC
  ${CMP_BASE}/COM_compositor.h
  ${CMP_BASE}/COM_defines.h

  #intern
  ${CMP_BASE}/intern/COM_CPUDevice.cpp
  ${CMP_BASE}/intern/COM_CPUDevice.h
  ${CMP_BASE}/intern/COM_ExecutionManager.cpp
  ${CMP_BASE}/intern/COM_ExecutionManager.h
  ${CMP_BASE}/intern/COM_CompositorContext.cpp
  ${CMP_BASE}/intern/COM_CompositorContext.h
  ${CMP_BASE}/intern/COM_Converter.cpp
  ${CMP_BASE}/intern/COM_Converter.h
  ${CMP_BASE}/intern/COM_Debug.cpp
  ${CMP_BASE}/intern/COM_Debug.h
  ${CMP_BASE}/intern/COM_ExecutionGroup.cpp
  ${CMP_BASE}/intern/COM_ExecutionGroup.h
  ${CMP_BASE}/intern/COM_ExecutionSystem.cpp
  ${CMP_BASE}/intern/COM_ExecutionSystem.h
  ${CMP_BASE}/intern/COM_GlobalManager.cpp
  ${CMP_BASE}/intern/COM_GlobalManager.h
  ${CMP_BASE}/intern/COM_Keys.h
  ${CMP_BASE}/intern/COM_Node.cpp
  ${CMP_BASE}/intern/COM_Node.h
  ${CMP_BASE}/intern/COM_NodeConverter.cpp
  ${CMP_BASE}/intern/COM_NodeConverter.h
  ${CMP_BASE}/intern/COM_NodeGraph.cpp
  ${CMP_BASE}/intern/COM_NodeGraph.h
  ${CMP_BASE}/intern/COM_NodeOperation.cpp
  ${CMP_BASE}/intern/COM_NodeOperation.h
  ${CMP_BASE}/intern/COM_NodeOperationBuilder.cpp
  ${CMP_BASE}/intern/COM_NodeOperationBuilder.h
  ${CMP_BASE}/intern/COM_NodeSocketReader.cpp
  ${CMP_BASE}/intern/COM_NodeSocketReader.h
  ${CMP_BASE}/intern/COM_WorkPackage.cpp
  ${CMP_BASE}/intern/COM_WorkPackage.h
  ${CMP_BASE}/intern/COM_WorkScheduler.cpp
  ${CMP_BASE}/intern/COM_WorkScheduler.h
  ${CMP_BASE}/intern/COM_compositor.cpp

  # buffering
  ${CMP_BASE}/buffering/COM_BufferManager.cpp
  ${CMP_BASE}/buffering/COM_BufferManager.h
  ${CMP_BASE}/buffering/COM_Buffer.h
  ${CMP_BASE}/buffering/COM_BufferRecycler.h
  ${CMP_BASE}/buffering/COM_BufferRecycler.cpp
  ${CMP_BASE}/buffering/COM_Rect.cpp
  ${CMP_BASE}/buffering/COM_Rect.h
  ${CMP_BASE}/buffering/COM_Pixels.cpp
  ${CMP_BASE}/buffering/COM_Pixels.h
  ${CMP_BASE}/buffering/COM_ReadsOptimizer.h
  ${CMP_BASE}/buffering/COM_ReadsOptimizer.cpp

  #caching
  ${CMP_BASE}/caching/COM_BaseCache.cpp
  ${CMP_BASE}/caching/COM_BaseCache.h
  ${CMP_BASE}/caching/COM_CacheManager.cpp
  ${CMP_BASE}/caching/COM_CacheManager.h
  ${CMP_BASE}/caching/COM_DiskCache.cpp
  ${CMP_BASE}/caching/COM_DiskCache.h
  ${CMP_BASE}/caching/COM_MemoryCache.cpp
  ${CMP_BASE}/caching/COM_MemoryCache.h
  ${CMP_BASE}/caching/COM_ViewCacheManager.cpp
  ${CMP_BASE}/caching/COM_ViewCacheManager.h

  # util
  ${CMP_BASE}/util/COM_BaseConverter.cpp
  ${CMP_BASE}/util/COM_BaseConverter.h
  ${CMP_BASE}/util/COM_PixelsUtil.cpp
  ${CMP_BASE}/util/COM_PixelsUtil.h
  ${CMP_BASE}/util/COM_RectUtil.cpp
  ${CMP_BASE}/util/COM_RectUtil.h
  ${CMP_BASE}/util/COM_MathUtil.cpp
  ${CMP_BASE}/util/COM_MathUtil.h
  ${CMP_BASE}/util/COM_TimeUtil.cpp
  ${CMP_BASE}/util/COM_TimeUtil.h
  ${CMP_BASE}/util/COM_Renderer.cpp
  ${CMP_BASE}/util/COM_Renderer.h
  ${CMP_BASE}/util/COM_StringUtil.cpp
  ${CMP_BASE}/util/COM_StringUtil.h
  ${CMP_BASE}/util/COM_BufferUtil.cpp
  ${CMP_BASE}/util/COM_BufferUtil.h
  ${CMP_BASE}/util/COM_UiConvert.cpp
  ${CMP_BASE}/util/COM_UiConvert.h

  # computing
  ${CMP_BASE}/computing/COM_ComputeDevice.cpp
  ${CMP_BASE}/computing/COM_ComputeDevice.h
  ${CMP_BASE}/computing/COM_ComputeKernel.cpp
  ${CMP_BASE}/computing/COM_ComputeKernel.h
  ${CMP_BASE}/computing/COM_ComputePlatform.cpp
  ${CMP_BASE}/computing/COM_ComputePlatform.h
  ${CMP_BASE}/computing/COM_ComputeManager.cpp
  ${CMP_BASE}/computing/COM_ComputeManager.h
  ${CMP_BASE}/computing/COM_ComputeNoneManager.h
  ${CMP_BASE}/computing/COM_kernel_cpu.h
  ${CMP_BASE}/computing/COM_kernel_opencl.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_algo.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_defines.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_color.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_geom.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_filter.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_math.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_math_float2.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_math_float3.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_math_float4.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_math_int2.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_math_int3.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_math_int4.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_optimization.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_random.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_sampling.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_sampling_impl.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_simd.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_simd.cpp
  ${CMP_BASE}/computing/kernel_util/COM_kernel_types.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_types_float2.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_types_float2_impl.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_types_float3.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_types_float3_impl.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_types_float4.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_types_float4_impl.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_types_int2.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_types_int2_impl.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_types_int3.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_types_int3_impl.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_types_int4.h
  ${CMP_BASE}/computing/kernel_util/COM_kernel_types_int4_impl.h

  # opencl
  ${CMP_BASE}/computing/opencl/COM_OpenCLManager.cpp
  ${CMP_BASE}/computing/opencl/COM_OpenCLManager.h
  ${CMP_BASE}/computing/opencl/COM_OpenCLDevice.cpp
  ${CMP_BASE}/computing/opencl/COM_OpenCLDevice.h
  ${CMP_BASE}/computing/opencl/COM_OpenCLKernel.cpp
  ${CMP_BASE}/computing/opencl/COM_OpenCLKernel.h
  ${CMP_BASE}/computing/opencl/COM_OpenCLPlatform.cpp
  ${CMP_BASE}/computing/opencl/COM_OpenCLPlatform.h

  # Internal nodes
  ${CMP_NODES}/COM_SocketProxyNode.cpp
  ${CMP_NODES}/COM_SocketProxyNode.h

  # input nodes
  ${CMP_NODES}/input/COM_BokehImageNode.cpp
  ${CMP_NODES}/input/COM_BokehImageNode.h
  ${CMP_NODES}/input/COM_ColorNode.cpp
  ${CMP_NODES}/input/COM_ColorNode.h
  ${CMP_NODES}/input/COM_ImageNode.cpp
  ${CMP_NODES}/input/COM_ImageNode.h
  ${CMP_NODES}/input/COM_MaskNode.cpp
  ${CMP_NODES}/input/COM_MaskNode.h
  ${CMP_NODES}/input/COM_CacheNode.cpp
  ${CMP_NODES}/input/COM_CacheNode.h
  ${CMP_NODES}/input/COM_MovieClipNode.cpp
  ${CMP_NODES}/input/COM_MovieClipNode.h
  ${CMP_NODES}/input/COM_RenderLayersNode.cpp
  ${CMP_NODES}/input/COM_RenderLayersNode.h
  ${CMP_NODES}/input/COM_TextureNode.cpp
  ${CMP_NODES}/input/COM_TextureNode.h
  ${CMP_NODES}/input/COM_TimeNode.cpp
  ${CMP_NODES}/input/COM_TimeNode.h
  ${CMP_NODES}/input/COM_TrackPositionNode.cpp
  ${CMP_NODES}/input/COM_TrackPositionNode.h
  ${CMP_NODES}/input/COM_ValueNode.cpp
  ${CMP_NODES}/input/COM_ValueNode.h
  ${CMP_NODES}/input/COM_VideoSequencerNode.cpp
  ${CMP_NODES}/input/COM_VideoSequencerNode.h

  # output nodes
  ${CMP_NODES}/output/COM_CompositorNode.cpp
  ${CMP_NODES}/output/COM_CompositorNode.h
  ${CMP_NODES}/output/COM_OutputFileNode.cpp
  ${CMP_NODES}/output/COM_OutputFileNode.h
  ${CMP_NODES}/output/COM_SplitViewerNode.cpp
  ${CMP_NODES}/output/COM_SplitViewerNode.h
  ${CMP_NODES}/output/COM_ViewerNode.cpp
  ${CMP_NODES}/output/COM_ViewerNode.h
  ${CMP_NODES}/output/COM_ViewLevelsNode.cpp
  ${CMP_NODES}/output/COM_ViewLevelsNode.h

  # color nodes
  ${CMP_NODES}/color/COM_AlphaOverNode.cpp
  ${CMP_NODES}/color/COM_AlphaOverNode.h
  ${CMP_NODES}/color/COM_BrightnessNode.cpp
  ${CMP_NODES}/color/COM_BrightnessNode.h
  ${CMP_NODES}/color/COM_ColorBalanceNode.cpp
  ${CMP_NODES}/color/COM_ColorBalanceNode.h
  ${CMP_NODES}/color/COM_ColorCorrectionNode.cpp
  ${CMP_NODES}/color/COM_ColorCorrectionNode.h
  ${CMP_NODES}/color/COM_ColorCurveNode.cpp
  ${CMP_NODES}/color/COM_ColorCurveNode.h
  ${CMP_NODES}/color/COM_GammaNode.cpp
  ${CMP_NODES}/color/COM_GammaNode.h
  ${CMP_NODES}/color/COM_HueSaturationValueCorrectNode.cpp
  ${CMP_NODES}/color/COM_HueSaturationValueCorrectNode.h
  ${CMP_NODES}/color/COM_HueSaturationValueNode.cpp
  ${CMP_NODES}/color/COM_HueSaturationValueNode.h
  ${CMP_NODES}/color/COM_InvertNode.cpp
  ${CMP_NODES}/color/COM_InvertNode.h
  ${CMP_NODES}/color/COM_MixNode.cpp
  ${CMP_NODES}/color/COM_MixNode.h
  ${CMP_NODES}/color/COM_TonemapNode.cpp
  ${CMP_NODES}/color/COM_TonemapNode.h
  ${CMP_NODES}/color/COM_ZCombineNode.cpp
  ${CMP_NODES}/color/COM_ZCombineNode.h

  # converter nodes
  ${CMP_NODES}/converter/COM_ColorRampNode.cpp
  ${CMP_NODES}/converter/COM_ColorRampNode.h
  ${CMP_NODES}/converter/COM_ColorToBWNode.cpp
  ${CMP_NODES}/converter/COM_ColorToBWNode.h
  ${CMP_NODES}/converter/COM_CombineColorNode.cpp
  ${CMP_NODES}/converter/COM_CombineColorNode.h
  ${CMP_NODES}/converter/COM_ConvertAlphaNode.cpp
  ${CMP_NODES}/converter/COM_ConvertAlphaNode.h
  ${CMP_NODES}/converter/COM_IDMaskNode.cpp
  ${CMP_NODES}/converter/COM_IDMaskNode.h
  ${CMP_NODES}/converter/COM_MathNode.cpp
  ${CMP_NODES}/converter/COM_MathNode.h
  ${CMP_NODES}/converter/COM_SeparateColorNode.cpp
  ${CMP_NODES}/converter/COM_SeparateColorNode.h
  ${CMP_NODES}/converter/COM_SetAlphaNode.cpp
  ${CMP_NODES}/converter/COM_SetAlphaNode.h
  ${CMP_NODES}/converter/COM_SwitchViewNode.cpp
  ${CMP_NODES}/converter/COM_SwitchViewNode.h

  # filter nodes
  ${CMP_NODES}/filter/COM_BilateralBlurNode.cpp
  ${CMP_NODES}/filter/COM_BilateralBlurNode.h
  ${CMP_NODES}/filter/COM_BlurNode.cpp
  ${CMP_NODES}/filter/COM_BlurNode.h
  ${CMP_NODES}/filter/COM_BokehBlurNode.cpp
  ${CMP_NODES}/filter/COM_BokehBlurNode.h
  ${CMP_NODES}/filter/COM_DefocusNode.cpp
  ${CMP_NODES}/filter/COM_DefocusNode.h
  ${CMP_NODES}/filter/COM_DenoiseNode.cpp
  ${CMP_NODES}/filter/COM_DenoiseNode.h
  ${CMP_NODES}/filter/COM_DespeckleNode.cpp
  ${CMP_NODES}/filter/COM_DespeckleNode.h
  ${CMP_NODES}/filter/COM_DilateErodeNode.cpp
  ${CMP_NODES}/filter/COM_DilateErodeNode.h
  ${CMP_NODES}/filter/COM_DirectionalBlurNode.cpp
  ${CMP_NODES}/filter/COM_DirectionalBlurNode.h
  ${CMP_NODES}/filter/COM_FilterNode.cpp
  ${CMP_NODES}/filter/COM_FilterNode.h
  ${CMP_NODES}/filter/COM_GlareNode.cpp
  ${CMP_NODES}/filter/COM_GlareNode.h
  ${CMP_NODES}/filter/COM_InpaintNode.cpp
  ${CMP_NODES}/filter/COM_InpaintNode.h
  ${CMP_NODES}/filter/COM_PixelateNode.cpp
  ${CMP_NODES}/filter/COM_PixelateNode.h
  ${CMP_NODES}/filter/COM_SunBeamsNode.cpp
  ${CMP_NODES}/filter/COM_SunBeamsNode.h
  ${CMP_NODES}/filter/COM_VectorBlurNode.cpp
  ${CMP_NODES}/filter/COM_VectorBlurNode.h

  # vector nodes
  ${CMP_NODES}/vector/COM_MapRangeNode.cpp
  ${CMP_NODES}/vector/COM_MapRangeNode.h
  ${CMP_NODES}/vector/COM_MapValueNode.cpp
  ${CMP_NODES}/vector/COM_MapValueNode.h
  ${CMP_NODES}/vector/COM_NormalizeNode.cpp
  ${CMP_NODES}/vector/COM_NormalizeNode.h
  ${CMP_NODES}/vector/COM_NormalNode.cpp
  ${CMP_NODES}/vector/COM_NormalNode.h
  ${CMP_NODES}/vector/COM_VectorCurveNode.cpp
  ${CMP_NODES}/vector/COM_VectorCurveNode.h

  # matte nodes
  ${CMP_NODES}/matte/COM_BoxMaskNode.cpp
  ${CMP_NODES}/matte/COM_BoxMaskNode.h
  ${CMP_NODES}/matte/COM_ChannelMatteNode.cpp
  ${CMP_NODES}/matte/COM_ChannelMatteNode.h
  ${CMP_NODES}/matte/COM_ChromaMatteNode.cpp
  ${CMP_NODES}/matte/COM_ChromaMatteNode.h
  ${CMP_NODES}/matte/COM_ColorMatteNode.cpp
  ${CMP_NODES}/matte/COM_ColorMatteNode.h
  ${CMP_NODES}/matte/COM_ColorSpillNode.cpp
  ${CMP_NODES}/matte/COM_ColorSpillNode.h
  ${CMP_NODES}/matte/COM_CryptomatteNode.cpp
  ${CMP_NODES}/matte/COM_CryptomatteNode.h
  ${CMP_NODES}/matte/COM_DifferenceMatteNode.cpp
  ${CMP_NODES}/matte/COM_DifferenceMatteNode.h
  ${CMP_NODES}/matte/COM_DistanceMatteNode.cpp
  ${CMP_NODES}/matte/COM_DistanceMatteNode.h
  ${CMP_NODES}/matte/COM_DoubleEdgeMaskNode.cpp
  ${CMP_NODES}/matte/COM_DoubleEdgeMaskNode.h
  ${CMP_NODES}/matte/COM_EllipseMaskNode.cpp
  ${CMP_NODES}/matte/COM_EllipseMaskNode.h
  ${CMP_NODES}/matte/COM_KeyingNode.cpp
  ${CMP_NODES}/matte/COM_KeyingNode.h
  ${CMP_NODES}/matte/COM_KeyingScreenNode.cpp
  ${CMP_NODES}/matte/COM_KeyingScreenNode.h
  ${CMP_NODES}/matte/COM_LuminanceMatteNode.cpp
  ${CMP_NODES}/matte/COM_LuminanceMatteNode.h

  # distort nodes
  ${CMP_NODES}/distort/COM_CornerPinNode.cpp
  ${CMP_NODES}/distort/COM_CornerPinNode.h
  ${CMP_NODES}/distort/COM_CropNode.cpp
  ${CMP_NODES}/distort/COM_CropNode.h
  ${CMP_NODES}/distort/COM_DisplaceNode.cpp
  ${CMP_NODES}/distort/COM_DisplaceNode.h
  ${CMP_NODES}/distort/COM_FlipNode.cpp
  ${CMP_NODES}/distort/COM_FlipNode.h
  ${CMP_NODES}/distort/COM_LensDistortionNode.cpp
  ${CMP_NODES}/distort/COM_LensDistortionNode.h
  ${CMP_NODES}/distort/COM_MapUVNode.cpp
  ${CMP_NODES}/distort/COM_MapUVNode.h
  ${CMP_NODES}/distort/COM_MovieDistortionNode.cpp
  ${CMP_NODES}/distort/COM_MovieDistortionNode.h
  ${CMP_NODES}/distort/COM_PlaneTrackDeformNode.cpp
  ${CMP_NODES}/distort/COM_PlaneTrackDeformNode.h
  ${CMP_NODES}/distort/COM_RotateNode.cpp
  ${CMP_NODES}/distort/COM_RotateNode.h
  ${CMP_NODES}/distort/COM_ScaleNode.cpp
  ${CMP_NODES}/distort/COM_ScaleNode.h
  ${CMP_NODES}/distort/COM_Stabilize2dNode.cpp
  ${CMP_NODES}/distort/COM_Stabilize2dNode.h
  ${CMP_NODES}/distort/COM_TransformNode.cpp
  ${CMP_NODES}/distort/COM_TransformNode.h
  ${CMP_NODES}/distort/COM_TranslateNode.cpp
  ${CMP_NODES}/distort/COM_TranslateNode.h

  # layout nodes
  ${CMP_NODES}/layout/COM_SwitchNode.cpp
  ${CMP_NODES}/layout/COM_SwitchNode.h

  # experimental nodes
  ${CMP_NODES}/experimental/COM_ExtendNode.cpp
  ${CMP_NODES}/experimental/COM_ExtendNode.h
  ${CMP_NODES}/experimental/COM_RandomizeNode.cpp
  ${CMP_NODES}/experimental/COM_RandomizeNode.h

  # Internal operations
  ${CMP_OPS}/COM_AntiAliasOperation.cpp
  ${CMP_OPS}/COM_AntiAliasOperation.h
  ${CMP_OPS}/COM_SocketProxyOperation.cpp
  ${CMP_OPS}/COM_SocketProxyOperation.h
  ${CMP_OPS}/COM_CurveBaseOperation.cpp
  ${CMP_OPS}/COM_CurveBaseOperation.h
  ${CMP_OPS}/COM_SetColorOperation.cpp
  ${CMP_OPS}/COM_SetColorOperation.h
  ${CMP_OPS}/COM_SetValueOperation.cpp
  ${CMP_OPS}/COM_SetValueOperation.h
  ${CMP_OPS}/COM_SetVectorOperation.cpp
  ${CMP_OPS}/COM_SetVectorOperation.h
  ${CMP_OPS}/COM_QualityStepHelper.cpp
  ${CMP_OPS}/COM_QualityStepHelper.h
  ${CMP_OPS}/COM_SingleElemReadyOperation.cpp
  ${CMP_OPS}/COM_SingleElemReadyOperation.h

  # input operations
  ${CMP_OPS}/input/COM_BokehImageOperation.cpp
  ${CMP_OPS}/input/COM_BokehImageOperation.h
  ${CMP_OPS}/input/COM_ImageOperation.cpp
  ${CMP_OPS}/input/COM_ImageOperation.h
  ${CMP_OPS}/input/COM_MaskOperation.cpp
  ${CMP_OPS}/input/COM_MaskOperation.h
  ${CMP_OPS}/input/COM_CacheOperation.cpp
  ${CMP_OPS}/input/COM_CacheOperation.h
  ${CMP_OPS}/input/COM_MovieClipOperation.cpp
  ${CMP_OPS}/input/COM_MovieClipOperation.h
  ${CMP_OPS}/input/COM_MultilayerImageOperation.cpp
  ${CMP_OPS}/input/COM_MultilayerImageOperation.h
  ${CMP_OPS}/input/COM_RenderLayersProg.cpp
  ${CMP_OPS}/input/COM_RenderLayersProg.h
  ${CMP_OPS}/input/COM_TextureOperation.cpp
  ${CMP_OPS}/input/COM_TextureOperation.h
  ${CMP_OPS}/input/COM_TrackPositionOperation.cpp
  ${CMP_OPS}/input/COM_TrackPositionOperation.h
  ${CMP_OPS}/input/COM_VideoSequencerOperation.cpp
  ${CMP_OPS}/input/COM_VideoSequencerOperation.h

  # output operations
  ${CMP_OPS}/output/COM_CalculateMeanOperation.cpp
  ${CMP_OPS}/output/COM_CalculateMeanOperation.h
  ${CMP_OPS}/output/COM_CalculateStandardDeviationOperation.cpp
  ${CMP_OPS}/output/COM_CalculateStandardDeviationOperation.h
  ${CMP_OPS}/output/COM_CompositorOperation.cpp
  ${CMP_OPS}/output/COM_CompositorOperation.h
  ${CMP_OPS}/output/COM_OutputFileMultiViewOperation.cpp
  ${CMP_OPS}/output/COM_OutputFileMultiViewOperation.h
  ${CMP_OPS}/output/COM_OutputFileOperation.cpp
  ${CMP_OPS}/output/COM_OutputFileOperation.h
  ${CMP_OPS}/output/COM_PreviewOperation.cpp
  ${CMP_OPS}/output/COM_PreviewOperation.h
  ${CMP_OPS}/output/COM_SplitOperation.cpp
  ${CMP_OPS}/output/COM_SplitOperation.h
  ${CMP_OPS}/output/COM_ViewerOperation.cpp
  ${CMP_OPS}/output/COM_ViewerOperation.h

  # color operations
  ${CMP_OPS}/color/COM_AlphaOverKeyOperation.cpp
  ${CMP_OPS}/color/COM_AlphaOverKeyOperation.h
  ${CMP_OPS}/color/COM_AlphaOverMixedOperation.cpp
  ${CMP_OPS}/color/COM_AlphaOverMixedOperation.h
  ${CMP_OPS}/color/COM_AlphaOverPremultiplyOperation.cpp
  ${CMP_OPS}/color/COM_AlphaOverPremultiplyOperation.h
  ${CMP_OPS}/color/COM_BrightnessOperation.cpp
  ${CMP_OPS}/color/COM_BrightnessOperation.h
  ${CMP_OPS}/color/COM_ChangeHSVOperation.cpp
  ${CMP_OPS}/color/COM_ChangeHSVOperation.h
  ${CMP_OPS}/color/COM_ColorBalanceASCCDLOperation.cpp
  ${CMP_OPS}/color/COM_ColorBalanceASCCDLOperation.h
  ${CMP_OPS}/color/COM_ColorBalanceLGGOperation.cpp
  ${CMP_OPS}/color/COM_ColorBalanceLGGOperation.h
  ${CMP_OPS}/color/COM_ColorCorrectionOperation.cpp
  ${CMP_OPS}/color/COM_ColorCorrectionOperation.h
  ${CMP_OPS}/color/COM_ColorCurveOperation.cpp
  ${CMP_OPS}/color/COM_ColorCurveOperation.h
  ${CMP_OPS}/color/COM_GammaOperation.cpp
  ${CMP_OPS}/color/COM_GammaOperation.h
  ${CMP_OPS}/color/COM_HueSaturationValueCorrectOperation.cpp
  ${CMP_OPS}/color/COM_HueSaturationValueCorrectOperation.h
  ${CMP_OPS}/color/COM_InvertOperation.cpp
  ${CMP_OPS}/color/COM_InvertOperation.h
  ${CMP_OPS}/color/COM_MixOperation.cpp
  ${CMP_OPS}/color/COM_MixOperation.h
  ${CMP_OPS}/color/COM_TonemapOperation.cpp
  ${CMP_OPS}/color/COM_TonemapOperation.h
  ${CMP_OPS}/color/COM_ZCombineOperation.cpp
  ${CMP_OPS}/color/COM_ZCombineOperation.h

  # converter operations
  ${CMP_OPS}/converter/COM_ColorRampOperation.cpp
  ${CMP_OPS}/converter/COM_ColorRampOperation.h
  ${CMP_OPS}/converter/COM_ConvertOperation.cpp
  ${CMP_OPS}/converter/COM_ConvertOperation.h
  ${CMP_OPS}/converter/COM_IDMaskOperation.cpp
  ${CMP_OPS}/converter/COM_IDMaskOperation.h
  ${CMP_OPS}/converter/COM_MathBaseOperation.cpp
  ${CMP_OPS}/converter/COM_MathBaseOperation.h
  ${CMP_OPS}/converter/COM_SetAlphaOperation.cpp
  ${CMP_OPS}/converter/COM_SetAlphaOperation.h

  # filter operations
  ${CMP_OPS}/filter/COM_BilateralBlurOperation.cpp
  ${CMP_OPS}/filter/COM_BilateralBlurOperation.h
  ${CMP_OPS}/filter/COM_BlurBaseOperation.cpp
  ${CMP_OPS}/filter/COM_BlurBaseOperation.h
  ${CMP_OPS}/filter/COM_BokehBlurOperation.cpp
  ${CMP_OPS}/filter/COM_BokehBlurOperation.h
  ${CMP_OPS}/filter/COM_ConvertDepthToRadiusOperation.cpp
  ${CMP_OPS}/filter/COM_ConvertDepthToRadiusOperation.h
  ${CMP_OPS}/filter/COM_ConvolutionEdgeFilterOperation.cpp
  ${CMP_OPS}/filter/COM_ConvolutionEdgeFilterOperation.h
  ${CMP_OPS}/filter/COM_ConvolutionFilterOperation.cpp
  ${CMP_OPS}/filter/COM_ConvolutionFilterOperation.h
  ${CMP_OPS}/filter/COM_DenoiseOperation.cpp
  ${CMP_OPS}/filter/COM_DenoiseOperation.h
  ${CMP_OPS}/filter/COM_DespeckleOperation.cpp
  ${CMP_OPS}/filter/COM_DespeckleOperation.h
  ${CMP_OPS}/filter/COM_DilateErodeOperation.cpp
  ${CMP_OPS}/filter/COM_DilateErodeOperation.h
  ${CMP_OPS}/filter/COM_DirectionalBlurOperation.cpp
  ${CMP_OPS}/filter/COM_DirectionalBlurOperation.h
  ${CMP_OPS}/filter/COM_FastGaussianBlurOperation.cpp
  ${CMP_OPS}/filter/COM_FastGaussianBlurOperation.h
  ${CMP_OPS}/filter/COM_GammaCorrectOperation.cpp
  ${CMP_OPS}/filter/COM_GammaCorrectOperation.h
  ${CMP_OPS}/filter/COM_GaussianBlurBaseOperation.cpp
  ${CMP_OPS}/filter/COM_GaussianBlurBaseOperation.h
  ${CMP_OPS}/filter/COM_GaussianAlphaXBlurOperation.cpp
  ${CMP_OPS}/filter/COM_GaussianAlphaXBlurOperation.h
  ${CMP_OPS}/filter/COM_GaussianAlphaYBlurOperation.cpp
  ${CMP_OPS}/filter/COM_GaussianAlphaYBlurOperation.h
  ${CMP_OPS}/filter/COM_GaussianBokehBlurOperation.cpp
  ${CMP_OPS}/filter/COM_GaussianBokehBlurOperation.h
  ${CMP_OPS}/filter/COM_GaussianXBlurOperation.cpp
  ${CMP_OPS}/filter/COM_GaussianXBlurOperation.h
  ${CMP_OPS}/filter/COM_GaussianYBlurOperation.cpp
  ${CMP_OPS}/filter/COM_GaussianYBlurOperation.h
  ${CMP_OPS}/filter/COM_GlareBaseOperation.cpp
  ${CMP_OPS}/filter/COM_GlareBaseOperation.h
  ${CMP_OPS}/filter/COM_GlareFogGlowOperation.cpp
  ${CMP_OPS}/filter/COM_GlareFogGlowOperation.h
  ${CMP_OPS}/filter/COM_GlareGhostOperation.cpp
  ${CMP_OPS}/filter/COM_GlareGhostOperation.h
  ${CMP_OPS}/filter/COM_GlareSimpleStarOperation.cpp
  ${CMP_OPS}/filter/COM_GlareSimpleStarOperation.h
  ${CMP_OPS}/filter/COM_GlareStreaksOperation.cpp
  ${CMP_OPS}/filter/COM_GlareStreaksOperation.h
  ${CMP_OPS}/filter/COM_GlareThresholdOperation.cpp
  ${CMP_OPS}/filter/COM_GlareThresholdOperation.h
  ${CMP_OPS}/filter/COM_InpaintOperation.cpp
  ${CMP_OPS}/filter/COM_InpaintOperation.h
  ${CMP_OPS}/filter/COM_KeyingBlurOperation.cpp
  ${CMP_OPS}/filter/COM_KeyingBlurOperation.h
  ${CMP_OPS}/filter/COM_PixelateOperation.cpp
  ${CMP_OPS}/filter/COM_PixelateOperation.h
  ${CMP_OPS}/filter/COM_SunBeamsOperation.cpp
  ${CMP_OPS}/filter/COM_SunBeamsOperation.h
  ${CMP_OPS}/filter/COM_VariableSizeBokehBlurOperation.cpp
  ${CMP_OPS}/filter/COM_VariableSizeBokehBlurOperation.h
  ${CMP_OPS}/filter/COM_VectorBlurOperation.cpp
  ${CMP_OPS}/filter/COM_VectorBlurOperation.h

  # vector operations
  ${CMP_OPS}/vector/COM_DotProductOperation.cpp
  ${CMP_OPS}/vector/COM_DotProductOperation.h
  ${CMP_OPS}/vector/COM_MapRangeOperation.cpp
  ${CMP_OPS}/vector/COM_MapRangeOperation.h
  ${CMP_OPS}/vector/COM_MapValueOperation.cpp
  ${CMP_OPS}/vector/COM_MapValueOperation.h
  ${CMP_OPS}/vector/COM_NormalizeOperation.cpp
  ${CMP_OPS}/vector/COM_NormalizeOperation.h
  ${CMP_OPS}/vector/COM_VectorCurveOperation.cpp
  ${CMP_OPS}/vector/COM_VectorCurveOperation.h

  # matte operations
  ${CMP_OPS}/matte/COM_BoxMaskOperation.cpp
  ${CMP_OPS}/matte/COM_BoxMaskOperation.h
  ${CMP_OPS}/matte/COM_ChannelMatteOperation.cpp
  ${CMP_OPS}/matte/COM_ChannelMatteOperation.h
  ${CMP_OPS}/matte/COM_ChromaMatteOperation.cpp
  ${CMP_OPS}/matte/COM_ChromaMatteOperation.h
  ${CMP_OPS}/matte/COM_ColorMatteOperation.cpp
  ${CMP_OPS}/matte/COM_ColorMatteOperation.h
  ${CMP_OPS}/matte/COM_ColorSpillOperation.cpp
  ${CMP_OPS}/matte/COM_ColorSpillOperation.h
  ${CMP_OPS}/matte/COM_CryptomatteOperation.cpp
  ${CMP_OPS}/matte/COM_CryptomatteOperation.h
  ${CMP_OPS}/matte/COM_DifferenceMatteOperation.cpp
  ${CMP_OPS}/matte/COM_DifferenceMatteOperation.h
  ${CMP_OPS}/matte/COM_DistanceRGBMatteOperation.cpp
  ${CMP_OPS}/matte/COM_DistanceRGBMatteOperation.h
  ${CMP_OPS}/matte/COM_DistanceYCCMatteOperation.cpp
  ${CMP_OPS}/matte/COM_DistanceYCCMatteOperation.h
  ${CMP_OPS}/matte/COM_DoubleEdgeMaskOperation.cpp
  ${CMP_OPS}/matte/COM_DoubleEdgeMaskOperation.h
  ${CMP_OPS}/matte/COM_EllipseMaskOperation.cpp
  ${CMP_OPS}/matte/COM_EllipseMaskOperation.h
  ${CMP_OPS}/matte/COM_KeyingClipOperation.cpp
  ${CMP_OPS}/matte/COM_KeyingClipOperation.h
  ${CMP_OPS}/matte/COM_KeyingDespillOperation.cpp
  ${CMP_OPS}/matte/COM_KeyingDespillOperation.h
  ${CMP_OPS}/matte/COM_KeyingOperation.cpp
  ${CMP_OPS}/matte/COM_KeyingOperation.h
  ${CMP_OPS}/matte/COM_KeyingScreenOperation.cpp
  ${CMP_OPS}/matte/COM_KeyingScreenOperation.h
  ${CMP_OPS}/matte/COM_LuminanceMatteOperation.cpp
  ${CMP_OPS}/matte/COM_LuminanceMatteOperation.h
  
  # distort operations
  ${CMP_OPS}/distort/COM_CropOperation.cpp
  ${CMP_OPS}/distort/COM_CropOperation.h
  ${CMP_OPS}/distort/COM_DisplaceOperation.cpp
  ${CMP_OPS}/distort/COM_DisplaceOperation.h
  ${CMP_OPS}/distort/COM_DisplaceSimpleOperation.cpp
  ${CMP_OPS}/distort/COM_DisplaceSimpleOperation.h
  ${CMP_OPS}/distort/COM_FlipOperation.cpp
  ${CMP_OPS}/distort/COM_FlipOperation.h
  ${CMP_OPS}/distort/COM_MapUVOperation.cpp
  ${CMP_OPS}/distort/COM_MapUVOperation.h
  ${CMP_OPS}/distort/COM_MovieClipAttributeOperation.cpp
  ${CMP_OPS}/distort/COM_MovieClipAttributeOperation.h
  ${CMP_OPS}/distort/COM_MovieDistortionOperation.cpp
  ${CMP_OPS}/distort/COM_MovieDistortionOperation.h
  ${CMP_OPS}/distort/COM_PlaneCornerPinOperation.cpp
  ${CMP_OPS}/distort/COM_PlaneCornerPinOperation.h
  ${CMP_OPS}/distort/COM_PlaneDistortCommonOperation.cpp
  ${CMP_OPS}/distort/COM_PlaneDistortCommonOperation.h
  ${CMP_OPS}/distort/COM_PlaneTrackOperation.cpp
  ${CMP_OPS}/distort/COM_PlaneTrackOperation.h
  ${CMP_OPS}/distort/COM_ProjectorLensDistortionOperation.cpp
  ${CMP_OPS}/distort/COM_ProjectorLensDistortionOperation.h
  ${CMP_OPS}/distort/COM_RotateOperation.cpp
  ${CMP_OPS}/distort/COM_RotateOperation.h
  ${CMP_OPS}/distort/COM_ScaleOperation.cpp
  ${CMP_OPS}/distort/COM_ScaleOperation.h
  ${CMP_OPS}/distort/COM_ScreenLensDistortionOperation.cpp
  ${CMP_OPS}/distort/COM_ScreenLensDistortionOperation.h
  ${CMP_OPS}/distort/COM_TranslateOperation.cpp
  ${CMP_OPS}/distort/COM_TranslateOperation.h
  ${CMP_OPS}/distort/COM_TransformOperation.cpp
  ${CMP_OPS}/distort/COM_TransformOperation.h

  # experimental operations
  ${CMP_OPS}/experimental/COM_ExtendOperation.cpp
  ${CMP_OPS}/experimental/COM_ExtendOperation.h
  ${CMP_OPS}/experimental/COM_RandomizeOperation.cpp
  ${CMP_OPS}/experimental/COM_RandomizeOperation.h

)

source_group(TREE ${CMP_BASE} FILES ${SRC})

set(LIB
  bf_defmerger
  bf_blenkernel
  bf_blenlib
  bf_depsgraph
  cycles_util
  extern_clew
  ${BOOST_LIBRARIES}
)

add_definitions(-DCL_USE_DEPRECATED_OPENCL_1_1_APIS)

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

# Merge OpenCL code
set(OPENCL_CODE_PATHS 
  ${CMP_OPS}
)
def_merger(
  OPENCL_CODE
  ${CMP_BASE}/computing/COM_OpenCLKernels.cl
  COM_kernel_opencl.h
  SRC
  OPENCL_CODE_PATHS
)
data_to_c(
  ${CMP_BASE}/computing/COM_OpenCLKernels.cl
  ${CMP_BASE}/computing/COM_OpenCLKernels.cl.h
  SRC
)

blender_add_lib(bf_compositor "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")

