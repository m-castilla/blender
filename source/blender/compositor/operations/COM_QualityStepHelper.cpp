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

#include "COM_QualityStepHelper.h"

QualityStepHelper::QualityStepHelper()
{
  this->m_quality = CompositorQuality::HIGH;
  this->m_step = 1;
}

void QualityStepHelper::initExecution(QualityHelper helper)
{
  switch (helper) {
    case QualityHelper::INCREASE:
      switch (this->m_quality) {
        case CompositorQuality::HIGH:
        default:
          this->m_step = 1;
          break;
        case CompositorQuality::MEDIUM:
          this->m_step = 2;
          break;
        case CompositorQuality::LOW:
          this->m_step = 3;
          break;
      }
      break;
    case QualityHelper::MULTIPLY:
      switch (this->m_quality) {
        case CompositorQuality::HIGH:
        default:
          this->m_step = 1;
          break;
        case CompositorQuality::MEDIUM:
          this->m_step = 2;
          break;
        case CompositorQuality::LOW:
          this->m_step = 4;
          break;
      }
      break;
  }
}
