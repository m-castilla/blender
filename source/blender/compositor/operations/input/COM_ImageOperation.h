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

#include "BKE_image.h"
#include "BLI_listbase.h"
#include "BLI_utildefines.h"
#include "COM_NodeOperation.h"
#include "MEM_guardedalloc.h"

#include "RE_pipeline.h"
#include "RE_render_ext.h"
#include "RE_shader_ext.h"

/**
 * \brief Base class for all image operations
 */
class BaseImageOperation : public NodeOperation {
 protected:
  ImBuf *m_im_buffer;
  Image *m_image;
  ImageUser *m_imageUser;
  float *m_imageFloatBuffer;
  unsigned int *m_imageByteBuffer;
  float *m_depthBuffer;
  int m_imageheight;
  int m_imagewidth;
  int m_framenumber;
  int m_numberOfChannels;
  const RenderData *m_rd;
  const char *m_viewName;

  BaseImageOperation();
  /**
   * Determine the output resolution. The resolution is retrieved from the Renderer
   */
  virtual ResolutionType determineResolution(int resolution[2],
                                             int preferredResolution[2],

                                             bool setResolution) override;

  virtual ImBuf *assureImBuf();

 public:
  void initExecution();
  void deinitExecution();
  void setImage(Image *image)
  {
    this->m_image = image;
  }
  void setImageUser(ImageUser *imageuser)
  {
    this->m_imageUser = imageuser;
  }
  void setRenderData(const RenderData *rd)
  {
    this->m_rd = rd;
  }
  void setViewName(const char *viewName)
  {
    this->m_viewName = viewName;
  }
  void setFramenumber(int framenumber)
  {
    this->m_framenumber = framenumber;
  }

 protected:
  virtual void hashParams() override;
};
class ImageOperation : public BaseImageOperation {
 public:
  /**
   * Constructor
   */
  ImageOperation();

 protected:
  void execPixels(ExecutionManager &man) override;
  bool canCompute() const override
  {
    return false;
  }
  void hashParams() override;
};
class ImageAlphaOperation : public BaseImageOperation {
 public:
  /**
   * Constructor
   */
  ImageAlphaOperation();

 protected:
  void execPixels(ExecutionManager &man) override;
  bool canCompute() const override
  {
    return false;
  }
  void hashParams() override;
};
class ImageDepthOperation : public BaseImageOperation {
 public:
  /**
   * Constructor
   */
  ImageDepthOperation();

 protected:
  void execPixels(ExecutionManager &man) override;
  bool canCompute() const override
  {
    return false;
  }
  void hashParams() override;
};
