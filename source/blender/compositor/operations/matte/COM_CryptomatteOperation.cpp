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
 * Copyright 2018, Blender Foundation.
 */

#include "COM_CryptomatteOperation.h"
#include "COM_kernel_cpu.h"

CryptomatteOperation::CryptomatteOperation(size_t num_inputs) : NodeOperation()
{
  for (size_t i = 0; i < num_inputs; i++) {
    this->addInputSocket(SocketType::COLOR);
  }
  this->addOutputSocket(SocketType::COLOR);
}

void CryptomatteOperation::hashParams()
{
  NodeOperation::hashParams();
  for (float obj_idx : m_objectIndex) {
    hashParam(obj_idx);
  }
}

void CryptomatteOperation::addObjectIndex(float objectIndex)
{
  if (objectIndex != 0.0f) {
    m_objectIndex.push_back(objectIndex);
  }
}

void CryptomatteOperation::execPixels(ExecutionManager &man)
{
  int n_inputs = getNumberOfInputSockets();
  std::vector<std::shared_ptr<PixelsRect>> inputs;
  for (int i = 0; i < n_inputs; i++) {
    inputs.push_back(getInputOperation(i)->getPixels(this, man));
  }
  float uint32_max_inv = 1.0f / (float)UINT32_MAX;
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    WRITE_DECL(dst);
    CPU_LOOP_START(dst);

    CCL::float4 output = CCL::TRANSPARENT_PIXEL;
    for (size_t i = 0; i < inputs.size(); i++) {
      auto color = inputs[i];

      READ_DECL(color);
      COPY_COORDS(color, dst_coords);
      READ_IMG4(color, color_pix);

      if (i == 0) {
        /* Write the frontmost object as false color for picking. */
        output.x = color_pix.x;
        uint32_t m3hash = *(reinterpret_cast<uint32_t *>(&color_pix));
        //::memcpy(&m3hash, (float *)&color_pix, sizeof(uint32_t));

        /* Since the red channel is likely to be out of display range,
         * setting green and blue gives more meaningful images. */
        output.y = ((float)(m3hash << 8) * uint32_max_inv);
        output.z = ((float)(m3hash << 16) * uint32_max_inv);
      }
      for (size_t i = 0; i < m_objectIndex.size(); i++) {
        if (m_objectIndex[i] == color_pix.x) {
          output.w += color_pix.y;
        }
        if (m_objectIndex[i] == color_pix.z) {
          output.w += color_pix.w;
        }
      }
    }
    WRITE_IMG4(dst, output);

    CPU_LOOP_END;
  };

  return cpuWriteSeek(man, cpuWrite);
}
