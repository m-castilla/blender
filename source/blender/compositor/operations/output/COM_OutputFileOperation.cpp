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

#include "COM_OutputFileOperation.h"

#include <string.h>

#include "BLI_listbase.h"
#include "BLI_path_util.h"
#include "BLI_string.h"
#include "COM_BufferUtil.h"
#include "COM_PixelsUtil.h"

#include "BKE_global.h"
#include "BKE_image.h"
#include "BKE_main.h"
#include "BKE_scene.h"

#include "DNA_color_types.h"
#include "DNA_scene_types.h"
#include "MEM_guardedalloc.h"

#include "IMB_colormanagement.h"
#include "IMB_imbuf.h"
#include "IMB_imbuf_types.h"

void add_exr_channels(void *exrhandle,
                      const char *layerName,
                      const DataType datatype,
                      const char *viewName,
                      const size_t width,
                      bool use_half_float,
                      float *buf)
{
  /* create channels */
  switch (datatype) {
    case DataType::VALUE:
      IMB_exr_add_channel(
          exrhandle, layerName, "V", viewName, 1, width, buf ? buf : NULL, use_half_float);
      break;
    case DataType::VECTOR:
      IMB_exr_add_channel(
          exrhandle, layerName, "X", viewName, 3, 3 * width, buf ? buf : NULL, use_half_float);
      IMB_exr_add_channel(
          exrhandle, layerName, "Y", viewName, 3, 3 * width, buf ? buf + 1 : NULL, use_half_float);
      IMB_exr_add_channel(
          exrhandle, layerName, "Z", viewName, 3, 3 * width, buf ? buf + 2 : NULL, use_half_float);
      break;
    case DataType::COLOR:
      IMB_exr_add_channel(
          exrhandle, layerName, "R", viewName, 4, 4 * width, buf ? buf : NULL, use_half_float);
      IMB_exr_add_channel(
          exrhandle, layerName, "G", viewName, 4, 4 * width, buf ? buf + 1 : NULL, use_half_float);
      IMB_exr_add_channel(
          exrhandle, layerName, "B", viewName, 4, 4 * width, buf ? buf + 2 : NULL, use_half_float);
      IMB_exr_add_channel(
          exrhandle, layerName, "A", viewName, 4, 4 * width, buf ? buf + 3 : NULL, use_half_float);
      break;
    default:
      break;
  }
}

void free_exr_channels(void *exrhandle,
                       const RenderData *rd,
                       const char *layerName,
                       const DataType datatype)
{
  SceneRenderView *srv;

  /* check renderdata for amount of views */
  for (srv = (SceneRenderView *)rd->views.first; srv; srv = srv->next) {
    float *rect = NULL;

    if (BKE_scene_multiview_is_render_view_active(rd, srv) == false) {
      continue;
    }

    /* the pointer is stored in the first channel of each datatype */
    switch (datatype) {
      case DataType::VALUE:
        rect = IMB_exr_channel_rect(exrhandle, layerName, "V", srv->name);
        break;
      case DataType::VECTOR:
        rect = IMB_exr_channel_rect(exrhandle, layerName, "X", srv->name);
        break;
      case DataType::COLOR:
        rect = IMB_exr_channel_rect(exrhandle, layerName, "R", srv->name);
        break;
      default:
        break;
    }
    if (rect) {
      MEM_freeN(rect);
    }
  }
}

static float *init_buffer(unsigned int width, unsigned int height, DataType datatype)
{
  // When initializing the tree during initial load the width and height can be zero.
  if (width != 0 && height != 0) {
    int size = PixelsUtil::getNChannels(datatype);
    return (float *)MEM_mallocN((size_t)width * height * size * sizeof(float),
                                "OutputFile buffer");
  }
  else {
    return NULL;
  }
}

OutputSingleLayerOperation::OutputSingleLayerOperation(
    const RenderData *rd,
    const bNodeTree *tree,
    DataType datatype,
    ImageFormatData *format,
    const char *path,
    const ColorManagedViewSettings *viewSettings,
    const ColorManagedDisplaySettings *displaySettings,
    const char *viewName)
{
  this->m_rd = rd;
  this->m_tree = tree;

  m_datatype = datatype;
  this->addInputSocket(PixelsUtil::dataToSocketType(datatype));

  this->m_outputBuffer = NULL;

  this->m_format = format;
  BLI_strncpy(this->m_path, path, sizeof(this->m_path));

  this->m_viewSettings = viewSettings;
  this->m_displaySettings = displaySettings;
  this->m_viewName = viewName;
}

void OutputSingleLayerOperation::initExecution()
{
  this->m_outputBuffer = init_buffer(this->getWidth(), this->getHeight(), m_datatype);
}

void OutputSingleLayerOperation::execPixels(ExecutionManager &man)
{
  auto img_pixels = getInputOperation(0)->getPixels(this, man);
  int n_channels = PixelsUtil::getNChannels(m_datatype);
  auto cpuWrite = [&, this](PixelsRect &dst, const WriteRectContext &ctx) {
    auto out_buffer = BufferUtil::createUnmanagedTmpBuffer(
        m_outputBuffer, getWidth(), getHeight(), n_channels, false);
    PixelsRect out_rect(out_buffer.get(), dst);
    PixelsRect img_rect = img_pixels->toRect(dst);
    PixelsUtil::copyEqualRects(out_rect, img_rect);
  };
  cpuWriteSeek(man, cpuWrite);
}

void OutputSingleLayerOperation::deinitExecution()
{
  if (this->getWidth() * this->getHeight() != 0) {

    ImBuf *ibuf = IMB_allocImBuf(this->getWidth(), this->getHeight(), this->m_format->planes, 0);
    char filename[FILE_MAX];
    const char *suffix;

    ibuf->channels = PixelsUtil::getNChannels(m_datatype);
    ibuf->rect_float = this->m_outputBuffer;
    ibuf->mall |= IB_rectfloat;
    ibuf->dither = this->m_rd->dither_intensity;

    IMB_colormanagement_imbuf_for_write(
        ibuf, true, false, m_viewSettings, m_displaySettings, this->m_format);

    suffix = BKE_scene_multiview_view_suffix_get(this->m_rd, this->m_viewName);

    BKE_image_path_from_imformat(filename,
                                 this->m_path,
                                 BKE_main_blendfile_path_from_global(),
                                 this->m_rd->cfra,
                                 this->m_format,
                                 (this->m_rd->scemode & R_EXTENSION) != 0,
                                 true,
                                 suffix);

    if (0 == BKE_imbuf_write(ibuf, filename, this->m_format)) {
      printf("Cannot save Node File Output to %s\n", filename);
    }
    else {
      printf("Saved: %s\n", filename);
    }

    IMB_freeImBuf(ibuf);
  }
  this->m_outputBuffer = NULL;
}

/******************************* MultiLayer *******************************/

OutputOpenExrLayer::OutputOpenExrLayer(const char *name_, DataType datatype_, bool use_layer_)
{
  BLI_strncpy(this->name, name_, sizeof(this->name));
  this->datatype = datatype_;
  this->use_layer = use_layer_;

  /* these are created in initExecution */
  this->outputBuffer = 0;
  this->imageInput = 0;
}

OutputOpenExrMultiLayerOperation::OutputOpenExrMultiLayerOperation(const RenderData *rd,
                                                                   const bNodeTree *tree,
                                                                   const char *path,
                                                                   char exr_codec,
                                                                   bool exr_half_float,
                                                                   const char *viewName)
{
  this->m_rd = rd;
  this->m_tree = tree;

  BLI_strncpy(this->m_path, path, sizeof(this->m_path));
  this->m_exr_codec = exr_codec;
  this->m_exr_half_float = exr_half_float;
  this->m_viewName = viewName;
}

void OutputOpenExrMultiLayerOperation::add_layer(const char *name,
                                                 DataType datatype,
                                                 bool use_layer)
{
  this->addInputSocket(PixelsUtil::dataToSocketType(datatype));
  this->m_layers.push_back(OutputOpenExrLayer(name, datatype, use_layer));
}

void OutputOpenExrMultiLayerOperation::initExecution()
{
  for (unsigned int i = 0; i < this->m_layers.size(); i++) {
    if (this->m_layers[i].use_layer) {
      NodeOperation *reader = getInputOperation(i);
      this->m_layers[i].imageInput = reader;
      this->m_layers[i].outputBuffer = init_buffer(
          this->getWidth(), this->getHeight(), this->m_layers[i].datatype);
    }
  }
}

void OutputOpenExrMultiLayerOperation::execPixels(ExecutionManager &man)
{
  std::vector<std::shared_ptr<PixelsRect>> layers_pixs;
  for (unsigned int i = 0; i < this->m_layers.size(); i++) {
    OutputOpenExrLayer &layer = this->m_layers[i];
    layers_pixs.push_back(layer.imageInput ? layer.imageInput->getPixels(this, man) : nullptr);
  }

  auto cpuWrite = [&, this](PixelsRect &dst, const WriteRectContext &ctx) {
    for (unsigned int i = 0; i < this->m_layers.size(); i++) {
      OutputOpenExrLayer &layer = this->m_layers[i];
      if (layer.imageInput) {
        auto &input_pixels = layers_pixs[i];
        int n_channels = PixelsUtil::getNChannels(layer.datatype);
        auto out_buffer = BufferUtil::createUnmanagedTmpBuffer(
            layer.outputBuffer, getWidth(), getHeight(), n_channels, false);

        PixelsRect input_rect = input_pixels->toRect(dst);
        PixelsRect out_rect(out_buffer.get(), dst);
        PixelsUtil::copyEqualRects(out_rect, input_rect);
      }
    }
  };
  cpuWriteSeek(man, cpuWrite);
}

void OutputOpenExrMultiLayerOperation::deinitExecution()
{
  unsigned int width = this->getWidth();
  unsigned int height = this->getHeight();
  if (width != 0 && height != 0) {
    char filename[FILE_MAX];
    const char *suffix;
    void *exrhandle = IMB_exr_get_handle();

    suffix = BKE_scene_multiview_view_suffix_get(this->m_rd, this->m_viewName);
    BKE_image_path_from_imtype(filename,
                               this->m_path,
                               BKE_main_blendfile_path_from_global(),
                               this->m_rd->cfra,
                               R_IMF_IMTYPE_MULTILAYER,
                               (this->m_rd->scemode & R_EXTENSION) != 0,
                               true,
                               suffix);
    BLI_make_existing_file(filename);

    for (unsigned int i = 0; i < this->m_layers.size(); i++) {
      OutputOpenExrLayer &layer = this->m_layers[i];
      if (!layer.imageInput) {
        continue; /* skip unconnected sockets */
      }

      add_exr_channels(exrhandle,
                       this->m_layers[i].name,
                       this->m_layers[i].datatype,
                       "",
                       width,
                       this->m_exr_half_float,
                       this->m_layers[i].outputBuffer);
    }

    /* when the filename has no permissions, this can fail */
    if (IMB_exr_begin_write(exrhandle, filename, width, height, this->m_exr_codec, NULL)) {
      IMB_exr_write_channels(exrhandle);
    }
    else {
      /* TODO, get the error from openexr's exception */
      /* XXX nice way to do report? */
      printf("Error Writing Render Result, see console\n");
    }

    IMB_exr_close(exrhandle);
    for (unsigned int i = 0; i < this->m_layers.size(); i++) {
      if (this->m_layers[i].outputBuffer) {
        MEM_freeN(this->m_layers[i].outputBuffer);
        this->m_layers[i].outputBuffer = NULL;
      }

      this->m_layers[i].imageInput = NULL;
    }
  }
}
