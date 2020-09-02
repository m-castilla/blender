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

#ifdef WITH_CXX_GUARDEDALLOC
#  include "MEM_guardedalloc.h"
#endif

#include "COM_Pixels.h"
#include "COM_defines.h"
#include "DNA_color_types.h"
#include "DNA_node_types.h"
#include "DNA_scene_types.h"
#include <string>
#include <vector>

/**
 * \brief Overall context of the compositor
 */
class CompositorContext {
 private:
  /**
   * \brief The rendering field describes if we are rendering (F12) or if we are editing (Node
   * editor) This field is initialized in ExecutionSystem and must only be read from that point on.
   * \see ExecutionSystem
   */
  bool m_rendering;

  /**
   * \brief The quality of the composite.
   * This field is initialized in ExecutionSystem and must only be read from that point on.
   * \see ExecutionSystem
   */
  CompositorQuality m_quality;

  struct Main *m_main;
  struct Depsgraph *m_depsgraph;
  ViewLayer *m_view_layer;
  Scene *m_scene;

  /**
   * \brief Reference to the render data that is being composited.
   * This field is initialized in ExecutionSystem and must only be read from that point on.
   * \see ExecutionSystem
   */
  RenderData *m_rd;

  /**
   * \brief reference to the bNodeTree
   * This field is initialized in ExecutionSystem and must only be read from that point on.
   * \see ExecutionSystem
   */
  bNodeTree *m_bnodetree;

  /**
   * \brief Preview image hash table
   * This field is initialized in ExecutionSystem and must only be read from that point on.
   */
  bNodeInstanceHash *m_previews;

  /* \brief color management settings */
  const ColorManagedViewSettings *m_viewSettings;
  const ColorManagedDisplaySettings *m_displaySettings;

  /**
   * \brief active rendering view name
   */
  const char *m_viewName;

  size_t m_max_cache_bytes;

  std::string m_execution_id;

  int m_cpu_work_threads;

  DetermineResolutionMode m_res_mode;

  float m_inputs_scale;

 private:
  CompositorContext();

 public:
  /**
   * \brief constructor initializes the context with default values.
   */
  static CompositorContext build(const std::string &execution_id,
                                 struct Main *main,
                                 struct Depsgraph *depsgraph,
                                 RenderData *rd,
                                 Scene *scene,
                                 ViewLayer *view_layer,
                                 bNodeTree *editingtree,
                                 bool rendering,
                                 const ColorManagedViewSettings *viewSettings,
                                 const ColorManagedDisplaySettings *displaySettings,
                                 const char *viewName);

  struct Main *getMain() const
  {
    return m_main;
  }
  void setMain(struct Main *main)
  {
    m_main = main;
  }

  struct Depsgraph *getDepsgraph() const
  {
    return m_depsgraph;
  }
  ViewLayer *getViewLayer() const
  {
    return m_view_layer;
  }
  void setViewLayer(ViewLayer *view_layer)
  {
    m_view_layer = view_layer;
  }
  void setDepsgraph(struct Depsgraph *depsgraph)
  {
    m_depsgraph = depsgraph;
  }

  PixelsSampler getDefaultSampler() const
  {
    return PixelsSampler{PixelInterpolation::BILINEAR, PixelExtend::CLIP};
  }

  PixelsSampler getViewsSampler() const
  {
    return PixelsSampler{PixelInterpolation::NEAREST, PixelExtend::CLIP};
  }

  bool isBreaked() const;
  int getPreviewSize() const;

  void setDetermineResolutionMode(DetermineResolutionMode mode)
  {
    m_res_mode = mode;
  }
  DetermineResolutionMode getDetermineResolutionMode() const
  {
    return m_res_mode;
  }

  float getInputsScale() const
  {
    return m_bnodetree->inputs_scale;
  }

  int getNCpuWorkThreads() const
  {
    return m_cpu_work_threads;
  }

  void setNCpuWorkThreads(int n_threads)
  {
    m_cpu_work_threads = n_threads;
  }
  void setExecutionId(const std::string &execution_id)
  {
    m_execution_id = execution_id;
  }

  std::string getExecutionId() const
  {
    return m_execution_id;
  }

  void setBufferCacheSize(size_t bytes)
  {
    m_max_cache_bytes = bytes;
  }

  size_t getBufferCacheSize() const
  {
    return m_max_cache_bytes;
  }

  /**
   * \brief set the rendering field of the context
   */
  void setRendering(bool rendering)
  {
    this->m_rendering = rendering;
  }

  /**
   * \brief get the rendering field of the context
   */
  bool isRendering() const
  {
    return this->m_rendering;
  }

  void setRenderData(RenderData *rd)
  {
    this->m_rd = rd;
  }

  const RenderData *getRenderData() const
  {
    return this->m_rd;
  }

  int getRenderWidth() const
  {
    return m_rd->xsch;
  }
  int getRenderHeight() const
  {
    return m_rd->ysch;
  }

  void setbNodeTree(bNodeTree *bnodetree)
  {
    this->m_bnodetree = bnodetree;
  }

  /**
   * \brief get the bnodetree of the context
   */
  bNodeTree *getbNodeTree() const
  {
    return this->m_bnodetree;
  }

  void setScene(Scene *scene)
  {
    m_scene = scene;
  }
  Scene *getScene() const
  {
    return m_scene;
  }

  /**
   * \brief set the preview image hash table
   */
  void setPreviewHash(bNodeInstanceHash *previews)
  {
    this->m_previews = previews;
  }

  /**
   * \brief get the preview image hash table
   */
  bNodeInstanceHash *getPreviewHash() const
  {
    return this->m_previews;
  }

  /**
   * \brief set view settings of color color management
   */
  void setViewSettings(const ColorManagedViewSettings *viewSettings)
  {
    this->m_viewSettings = viewSettings;
  }

  /**
   * \brief get view settings of color color management
   */
  const ColorManagedViewSettings *getViewSettings() const
  {
    return this->m_viewSettings;
  }

  /**
   * \brief set display settings of color color management
   */
  void setDisplaySettings(const ColorManagedDisplaySettings *displaySettings)
  {
    this->m_displaySettings = displaySettings;
  }

  /**
   * \brief get display settings of color color management
   */
  const ColorManagedDisplaySettings *getDisplaySettings() const
  {
    return this->m_displaySettings;
  }

  /**
   * \brief set the quality
   */
  void setQuality(CompositorQuality quality)
  {
    this->m_quality = quality;
  }

  /**
   * \brief get the quality
   */
  CompositorQuality getQuality() const
  {
    return this->m_quality;
  }

  /**
   * \brief get the current frame-number of the scene in this context
   */
  int getFramenumber() const;

  /**
   * \brief get the active rendering view
   */
  const char *getViewName() const
  {
    return this->m_viewName;
  }

  /**
   * \brief set the active rendering view
   */
  void setViewName(const char *viewName)
  {
    this->m_viewName = viewName;
  }

#ifdef WITH_CXX_GUARDEDALLOC
  MEM_CXX_CLASS_ALLOC_FUNCS("COM:CompositorContext")
#endif
};
