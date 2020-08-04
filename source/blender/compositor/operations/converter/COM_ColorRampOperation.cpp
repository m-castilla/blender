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

#include "COM_ColorRampOperation.h"

#include "BKE_colorband.h"

ColorRampOperation::ColorRampOperation() : NodeOperation()
{
  this->addInputSocket(COM_DT_VALUE);
  this->addOutputSocket(COM_DT_COLOR);

  this->m_inputProgram = NULL;
  this->m_colorBand = NULL;
}
void ColorRampOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_colorBand->color_mode);
  hashParam(m_colorBand->ipotype);
  hashParam(m_colorBand->ipotype_hue);
  hashParam(m_colorBand->tot);

  hashParam(m_colorBand->data->a);
  hashParam(m_colorBand->data->b);
  hashParam(m_colorBand->data->g);
  hashParam(m_colorBand->data->pos);
  hashParam(m_colorBand->data->r);
  /*No need to hash m_colorBand->cur and m_colorBand->data->cur variables, they are used as cursor
   * not parameters*/
}

void ColorRampOperation::initExecution()
{
  this->m_inputProgram = this->getInputSocketReader(0);
}

void ColorRampOperation::executePixelSampled(float output[4],
                                             float x,
                                             float y,
                                             PixelSampler sampler)
{
  float values[4];

  this->m_inputProgram->readSampled(values, x, y, sampler);
  BKE_colorband_evaluate(this->m_colorBand, values[0], output);
}

void ColorRampOperation::deinitExecution()
{
  this->m_inputProgram = NULL;
}
