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

#include "COM_CameraOperation.h"
#include "BKE_main.h"
#include "BKE_scene.h"
#include "BLI_listbase.h"
#include "COM_Buffer.h"
#include "COM_BufferUtil.h"
#include "COM_CompositorContext.h"
#include "COM_GlobalManager.h"
#include "COM_PixelsUtil.h"
#include "COM_Renderer.h"
#include "DNA_camera_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"
#include "IMB_imbuf_types.h"

#include "COM_kernel_cpu.h"

/* ******** Render Layers Base Prog ******** */

CameraOperation::CameraOperation() : NodeOperation()
{
  this->addOutputSocket(SocketType::COLOR);
  m_gl_render = nullptr;
}

void CameraOperation::hashParams()
{
  NodeOperation::hashParams();
  if (m_gl_render && m_gl_render->result) {
    hashParam(m_gl_render->ssid);
  }
}

void CameraOperation::execPixels(ExecutionManager &man)
{
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    if (m_gl_render == NULL || m_gl_render->result == NULL) {
      PixelsUtil::setRectElem(dst, (float *)&CCL::TRANSPARENT_PIXEL);
    }
    else {
      PixelsUtil::copyImBufRect(
          dst, m_gl_render->result, COM_NUM_CHANNELS_COLOR, COM_NUM_CHANNELS_COLOR);
    }
  };
  cpuWriteSeek(man, cpuWrite);
}

ResolutionType CameraOperation::determineResolution(int resolution[2],
                                                    int preferredResolution[2],
                                                    bool /*setResolution*/)
{
  if (m_gl_render && m_gl_render->result) {
    resolution[0] = m_gl_render->result->x;
    resolution[1] = m_gl_render->result->y;
  }
  else {
    resolution[0] = preferredResolution[0];
    resolution[1] = preferredResolution[1];
  }

  return ResolutionType::Fixed;
}
