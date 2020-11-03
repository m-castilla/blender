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

#include "BLI_listbase.h"
#include "BLI_utildefines.h"
#include "COM_NodeOperation.h"
#include "DNA_scene_types.h"
#include "MEM_guardedalloc.h"

#include "RE_pipeline.h"

struct ViewLayer;
/**
 * Base class for all renderlayeroperations
 */
class RenderLayersProg : public NodeOperation {
 protected:
  /**
   * Reference to the scene object.
   */
  Scene *m_scene;

  /**
   * layerId of the layer where this operation needs to get its data from
   */
  ViewLayer *m_view_layer;
  int m_layer_id;

  /**
   * viewName of the view to use (unless another view is specified by the node
   */
  const char *m_viewName;

  /**
   * cached instance to the float buffer inside the layer
   */
  float *m_inputBuffer;

  /**
   * Render-pass where this operation needs to get its data from.
   */
  std::string m_passName;

  int m_elementsize;

  /**
   * \brief render data used for active rendering
   */
  const RenderData *m_rd;

  /**
   * Determine the output resolution. The resolution is retrieved from the Renderer
   */
  ResolutionType determineResolution(int resolution[2],
                                     int preferredResolution[2],
                                     bool setResolution) override;

  /**
   * retrieve the reference to the float buffer of the renderer.
   */
  inline float *getInputBuffer()
  {
    return this->m_inputBuffer;
  }

  // void doInterpolation(float output[4], float x, float y, PixelSampler sampler);

 public:
  /**
   * Constructor
   */
  RenderLayersProg(const char *passName, DataType type, int elementsize);

  virtual bool canCompute() const override
  {
    return false;
  }

  virtual BufferType getBufferType() const override
  {
    return BufferType::CUSTOM;
  }
  virtual TmpBuffer *getCustomBuffer() override;
  /**
   * setter for the scene field. Will be called from
   * \see RenderLayerNode to set the actual scene where
   * the data will be retrieved from.
   */
  void setScene(Scene *scene)
  {
    this->m_scene = scene;
  }
  void setRenderData(const RenderData *rd)
  {
    this->m_rd = rd;
  }
  void setViewLayer(ViewLayer *view_layer)
  {
    this->m_view_layer = view_layer;
  }
  void setLayerId(int layer_id)
  {
    m_layer_id = layer_id;
  }
  void setViewName(const char *viewName)
  {
    this->m_viewName = viewName;
  }
  void initExecution();
  void deinitExecution();

 protected:
  virtual void hashParams();

 private:
  ViewLayer *getViewLayer();
};

class RenderLayersAlphaProg : public RenderLayersProg {
 public:
  RenderLayersAlphaProg(const char *passName, DataType type, int elementsize)
      : RenderLayersProg(passName, type, elementsize)
  {
  }
  virtual BufferType getBufferType() const override
  {
    return BufferType::TEMPORAL;
  }
  virtual void execPixels(ExecutionManager &man) override;
};

class RenderLayersDepthProg : public RenderLayersProg {
 public:
  RenderLayersDepthProg(const char *passName, DataType type, int elementsize)
      : RenderLayersProg(passName, type, elementsize)
  {
  }
};
