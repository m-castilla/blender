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

#include <algorithm>
#include <math.h>
#include <sstream>
#include <stdlib.h>

#include "COM_Debug.h"
#include "COM_ExecutionGroup.h"
#include "COM_ExecutionSystem.h"
#include "COM_ViewerOperation.h"
#include "COM_defines.h"

#include "BLI_math.h"
#include "BLI_string.h"
#include "BLT_translation.h"
#include "COM_CPUDevice.h"
#include "COM_RectUtil.h"
#include "MEM_guardedalloc.h"
#include "WM_api.h"
#include "WM_types.h"

ExecutionGroup::ExecutionGroup(ExecutionSystem &sys) : m_sys(sys)
{
  BLI_rcti_init(&this->m_viewerBorder, 0, 0, 0, 0);
}

bool ExecutionGroup::addOperation(NodeOperation *operation)
{

  m_operations.push_back(operation);
  return true;
}

NodeOperation *ExecutionGroup::getOutputOperation() const
{
  return this
      ->m_operations[0];  // the first operation of the group is always the output operation.
}

/**
 * this method is called for the top execution groups. containing the compositor node or the
 * preview node or the viewer node)
 */
void ExecutionGroup::execute(ExecutionManager &man)
{
  auto output_op = getOutputOperation();
  if (output_op->getWidth() == 0 || output_op->getHeight() == 0 || m_sys.isBreaked()) {
    return;
  }

  getOutputOperation()->getPixels(nullptr, man);
}

bool ExecutionGroup::hasOutputViewerBorder()
{
  return !BLI_rcti_is_empty(&m_viewerBorder);
}

void ExecutionGroup::setViewerBorder(float xmin, float xmax, float ymin, float ymax)
{
  NodeOperation *output_op = this->getOutputOperation();

  if (output_op->isViewerOperation() || output_op->isPreviewOperation()) {
    int width = output_op->getWidth();
    int height = output_op->getHeight();
    BLI_rcti_init(&this->m_viewerBorder, xmin * width, xmax * width, ymin * height, ymax * height);
  }
}

void ExecutionGroup::setRenderBorder(float xmin, float xmax, float ymin, float ymax)
{
  NodeOperation *output_op = this->getOutputOperation();

  if (output_op->isOutputOperation(true)) {
    /* Basically, setting border need to happen for only operations
     * which operates in render resolution buffers (like compositor
     * output nodes).
     *
     * In this cases adding border will lead to mapping coordinates
     * from output buffer space to input buffer spaces when executing
     * operation.
     *
     * But nodes like viewer and file output just shall display or
     * safe the same exact buffer which goes to their input, no need
     * in any kind of coordinates mapping.
     */

    bool operationNeedsBorder = !(output_op->isViewerOperation() ||
                                  output_op->isPreviewOperation() ||
                                  output_op->isFileOutputOperation());

    if (operationNeedsBorder) {
      int width = output_op->getWidth();
      int height = output_op->getHeight();
      BLI_rcti_init(
          &this->m_viewerBorder, xmin * width, xmax * width, ymin * height, ymax * height);
    }
  }
}
