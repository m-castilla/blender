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

#include "COM_BokehImageOperation.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"
#include "DNA_node_types.h"

using namespace std::placeholders;

BokehImageOperation::BokehImageOperation()
    : NodeOperation(),
      m_data(nullptr),
      m_center(),
      m_circularDistance(0),
      m_flapRad(0),
      m_flapRadAdd(0),
      m_deleteData(false)
{
  this->addOutputSocket(SocketType::COLOR);
}
void BokehImageOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_data->angle);
  hashParam(m_data->catadioptric);
  hashParam(m_data->flaps);
  hashParam(m_data->lensshift);
  hashParam(m_data->rounding);
}
void BokehImageOperation::initExecution()
{
  this->m_center[0] = getWidth() / 2;
  this->m_center[1] = getHeight() / 2;
  this->m_circularDistance = getWidth() / 2;
  this->m_flapRad = (float)(M_PI * 2) / this->m_data->flaps;
  this->m_flapRadAdd = this->m_data->angle;
  while (this->m_flapRadAdd < 0.0f) {
    this->m_flapRadAdd += (float)(M_PI * 2.0);
  }
  while (this->m_flapRadAdd > (float)M_PI) {
    this->m_flapRadAdd -= (float)(M_PI * 2.0);
  }
}

void BokehImageOperation::deinitExecution()
{
  if (this->m_deleteData) {
    if (this->m_data) {
      delete this->m_data;
      this->m_data = NULL;
    }
  }
}

void BokehImageOperation::determineResolution(int resolution[2],
                                              int /*preferredResolution*/[2],
                                              DetermineResolutionMode mode,
                                              bool setResolution)
{
  resolution[0] = COM_BLUR_BOKEH_PIXELS;
  resolution[1] = COM_BLUR_BOKEH_PIXELS;
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN

ccl_kernel bokehImageOp(CCL_WRITE(dst),
                        const float2 center,
                        const float circular_distance,
                        const float lensshift,
                        const float flap_rad,
                        const float flap_rad_add,
                        const float rounding,
                        const float catadioptric)
{
  float lensshift2 = lensshift / 2.0f;

  WRITE_DECL(dst);

  CPU_LOOP_START(dst);
  COORDS_TO_OFFSET(dst_coords);
  float2 dst_coordsf = make_float2(dst_coords.x, dst_coords.y);

  float insideBokehMax = bokehIsInside(
      circular_distance, dst_coordsf, center, flap_rad, flap_rad_add, rounding, catadioptric);
  float insideBokehMed = bokehIsInside(circular_distance - fabsf(lensshift2 * circular_distance),
                                       dst_coordsf,
                                       center,
                                       flap_rad,
                                       flap_rad_add,
                                       rounding,
                                       catadioptric);
  float insideBokehMin = bokehIsInside(circular_distance - fabsf(lensshift * circular_distance),
                                       dst_coordsf,
                                       center,
                                       flap_rad,
                                       flap_rad_add,
                                       rounding,
                                       catadioptric);

  float alpha = (insideBokehMax + insideBokehMed + insideBokehMin) / 3.0f;
  float4 result_pix = lensshift < 0 ?
                          make_float4(insideBokehMax, insideBokehMed, insideBokehMin, alpha) :
                          make_float4(insideBokehMin, insideBokehMed, insideBokehMax, alpha);

  WRITE_IMG(dst, dst_coords, result_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void BokehImageOperation::execPixels(ExecutionManager &man)
{
  CCL_NAMESPACE::float2 center = CCL_NAMESPACE::make_float2(m_center[0], m_center[1]);
  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL_NAMESPACE::bokehImageOp,
      _1,
      center,
      m_circularDistance,
      m_data->lensshift,
      m_flapRad,
      m_flapRadAdd,
      m_data->rounding,
      m_data->catadioptric);
  computeWriteSeek(man, cpu_write, "bokehImageOp", [&](ComputeKernel *kernel) {
    kernel->addFloat2Arg((float *)&center);
    kernel->addFloatArg(m_circularDistance);
    kernel->addFloatArg(m_data->lensshift);
    kernel->addFloatArg(m_flapRad);
    kernel->addFloatArg(m_flapRadAdd);
    kernel->addFloatArg(m_data->rounding);
    kernel->addFloatArg(m_data->catadioptric);
  });
}
