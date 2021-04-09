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

#include "COM_WriteBufferOperation.h"
#include "COM_defines.h"
#include <cstdio>

namespace blender::compositor {

WriteBufferOperation::WriteBufferOperation(DataType datatype)
{
  this->addInputSocket(datatype);
  this->m_memoryProxy = new MemoryProxy(datatype);
  this->m_memoryProxy->setWriteBufferOperation(this);
  this->m_memoryProxy->setExecutor(nullptr);
  flags.is_write_buffer_operation = true;
}
WriteBufferOperation::~WriteBufferOperation()
{
  if (this->m_memoryProxy) {
    delete this->m_memoryProxy;
    this->m_memoryProxy = nullptr;
  }
}

void WriteBufferOperation::executePixelSampled(float output[4],
                                               float x,
                                               float y,
                                               PixelSampler sampler)
{
  this->m_input->readSampled(output, x, y, sampler);
}

void WriteBufferOperation::initExecution()
{
  this->m_input = this->getInputOperation(0);
  this->m_memoryProxy->allocate(this->m_width, this->m_height);
}

void WriteBufferOperation::deinitExecution()
{
  this->m_input = nullptr;
  this->m_memoryProxy->free();
}

void WriteBufferOperation::executeRegion(rcti *rect, unsigned int /*tileNumber*/)
{
  MemoryBuffer *memoryBuffer = this->m_memoryProxy->getBuffer();
  float *buffer = memoryBuffer->getBuffer();
  const uint8_t num_channels = memoryBuffer->get_num_channels();
  if (this->m_input->get_flags().complex) {
    void *data = this->m_input->initializeTileData(rect);
    int x1 = rect->xmin;
    int y1 = rect->ymin;
    int x2 = rect->xmax;
    int y2 = rect->ymax;
    int x;
    int y;
    bool breaked = false;
    for (y = y1; y < y2 && (!breaked); y++) {
      int offset4 = (y * memoryBuffer->getWidth() + x1) * num_channels;
      for (x = x1; x < x2; x++) {
        this->m_input->read(&(buffer[offset4]), x, y, data);
        offset4 += num_channels;
      }
      if (isBraked()) {
        breaked = true;
      }
    }
    if (data) {
      this->m_input->deinitializeTileData(rect, data);
      data = nullptr;
    }
  }
  else {
    int x1 = rect->xmin;
    int y1 = rect->ymin;
    int x2 = rect->xmax;
    int y2 = rect->ymax;

    int x;
    int y;
    bool breaked = false;
    for (y = y1; y < y2 && (!breaked); y++) {
      int offset4 = (y * memoryBuffer->getWidth() + x1) * num_channels;
      for (x = x1; x < x2; x++) {
        this->m_input->readSampled(&(buffer[offset4]), x, y, PixelSampler::Nearest);
        offset4 += num_channels;
      }
      if (isBraked()) {
        breaked = true;
      }
    }
  }
}

void WriteBufferOperation::determineResolution(unsigned int resolution[2],
                                               unsigned int preferredResolution[2])
{
  NodeOperation::determineResolution(resolution, preferredResolution);
  /* make sure there is at least one pixel stored in case the input is a single value */
  m_single_value = false;
  if (resolution[0] == 0) {
    resolution[0] = 1;
    m_single_value = true;
  }
  if (resolution[1] == 0) {
    resolution[1] = 1;
    m_single_value = true;
  }
}

void WriteBufferOperation::readResolutionFromInputSocket()
{
  NodeOperation *inputOperation = this->getInputOperation(0);
  this->setWidth(inputOperation->getWidth());
  this->setHeight(inputOperation->getHeight());
}

}  // namespace blender::compositor
