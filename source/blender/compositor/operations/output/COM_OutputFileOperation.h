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

#include "COM_NodeOperation.h"

#include "BLI_path_util.h"
#include "BLI_rect.h"

#include "intern/openexr/openexr_multi.h"

struct RenderData;
struct ColorManagedViewSettings;
struct ColorManagedDisplaySettings;
struct ImageFormatData;
/* Writes the image to a single-layer file. */
class OutputSingleLayerOperation : public NodeOperation {
 protected:
  const RenderData *m_rd;
  const bNodeTree *m_tree;

  ImageFormatData *m_format;
  char m_path[FILE_MAX];

  float *m_outputBuffer;
  DataType m_datatype;

  const ColorManagedViewSettings *m_viewSettings;
  const ColorManagedDisplaySettings *m_displaySettings;

  const char *m_viewName;

 public:
  OutputSingleLayerOperation(const RenderData *rd,
                             const bNodeTree *tree,
                             DataType data_type,
                             ImageFormatData *format,
                             const char *path,
                             const ColorManagedViewSettings *viewSettings,
                             const ColorManagedDisplaySettings *displaySettings,
                             const char *viewName);

  bool isOutputOperation(bool /*rendering*/) const
  {
    return true;
  }
  void initExecution();
  void deinitExecution();

  bool isFileOutputOperation() const
  {
    return true;
  }

  bool canCompute() const override
  {
    return false;
  }
  BufferType getBufferType() const override
  {
    return BufferType::NO_BUFFER_WITH_WRITE;
  }

 protected:
  void execPixels(ExecutionManager &man) override;
};

/* extra info for OpenEXR layers */
struct OutputOpenExrLayer {
  OutputOpenExrLayer(const char *name, DataType datatype, bool use_layer);

  char name[EXR_TOT_MAXNAME - 2];
  DataType datatype;
  bool use_layer;

  /* internals */
  float *outputBuffer;
  NodeOperation *imageInput;
};

/* Writes inputs into OpenEXR multilayer channels. */
class OutputOpenExrMultiLayerOperation : public NodeOperation {
 protected:
  typedef std::vector<OutputOpenExrLayer> LayerList;

  const RenderData *m_rd;
  const bNodeTree *m_tree;

  char m_path[FILE_MAX];
  char m_exr_codec;
  bool m_exr_half_float;
  LayerList m_layers;
  const char *m_viewName;

 public:
  OutputOpenExrMultiLayerOperation(const RenderData *rd,
                                   const bNodeTree *tree,
                                   const char *path,
                                   char exr_codec,
                                   bool exr_half_float,
                                   const char *viewName);

  void add_layer(const char *name, DataType datatype, bool use_layer);

  bool isOutputOperation(bool /*rendering*/) const
  {
    return true;
  }
  void initExecution();
  void deinitExecution();

  bool isFileOutputOperation() const
  {
    return true;
  }
  bool canCompute() const override
  {
    return false;
  }
  BufferType getBufferType() const override
  {
    return BufferType::NO_BUFFER_WITH_WRITE;
  }

 protected:
  void execPixels(ExecutionManager &man) override;
};

void add_exr_channels(void *exrhandle,
                      const char *layerName,
                      const DataType datatype,
                      const char *viewName,
                      const size_t width,
                      bool use_half_float,
                      float *buf);
void free_exr_channels(void *exrhandle,
                       const RenderData *rd,
                       const char *layerName,
                       const DataType datatype);
