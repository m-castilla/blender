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
#include <unordered_map>
struct bNodePreview;
struct ColorManagedViewSettings;
struct ColorManagedDisplaySettings;
struct bNodeInstanceHash;
struct bNodeInstanceKey;
class CompositorContext;
class PreviewOperation : public NodeOperation {
 protected:
  unsigned char *m_outputBuffer;

  /**
   * \brief holds reference to the SDNA bNode, where this nodes will render the preview image for
   */
  bNodePreview *m_preview;
  bool m_needs_write;

  const ColorManagedViewSettings *m_viewSettings;
  const ColorManagedDisplaySettings *m_displaySettings;

 public:
  PreviewOperation(const ColorManagedViewSettings *viewSettings,
                   const ColorManagedDisplaySettings *displaySettings);
  void verifyPreview(bNodeInstanceHash *previews, bNodeInstanceKey key);

  bool canCompute() const override
  {
    return false;
  }
  int getOutputNUsedChannels() const override
  {
    return COM_NUM_CHANNELS_COLOR;
  }

  bool isOutputOperation(bool /*rendering*/) const override;
  void initExecution() override;
  void deinitExecution() override;
  void hashParams() override;

  /*void executeRegion(rcti *rect, unsigned int tileNumber);*/
  ResolutionType determineResolution(int resolution[2],
                                     int preferredResolution[2],
                                     bool setResolution) override;
  // bool determineDependingAreaOfInterest(rcti *input,
  //                                      ReadBufferOperation *readOperation,
  //                                      rcti *output);
  bool isPreviewOperation() const override
  {
    return true;
  }

  BufferType getBufferType() const override
  {
    return m_needs_write ? BufferType::TEMPORAL : BufferType::NO_BUFFER_NO_WRITE;
  }

  inline unsigned int getPreviewKey()
  {
    return m_preview->hash_entry.key.value;
  }

 protected:
  virtual void execPixels(ExecutionManager &man) override;

 private:
  unsigned char *createPreviewBuffer();
  size_t getBufferBytes()
  {
    return sizeof(unsigned char) * 4 * getWidth() * getHeight();
  }
};
