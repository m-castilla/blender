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

#include "COM_ConvertDepthToRadiusOperation.h"
#include "BKE_camera.h"
#include "COM_ComputeKernel.h"
#include "COM_kernel_cpu.h"
#include "DNA_camera_types.h"

using namespace std::placeholders;
ConvertDepthToRadiusOperation::ConvertDepthToRadiusOperation() : NodeOperation()
{
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(SocketType::VALUE);
  this->m_fStop = 128.0f;
  this->m_cameraObject = NULL;
  this->m_maxRadius = 32.0f;
  this->m_blurPostOperation = NULL;
}

void ConvertDepthToRadiusOperation::hashParams()
{
  NodeOperation::hashParams();
  /* Only these params affect output result */
  hashParam(m_maxRadius);
  hashParam(m_inverseFocalDistance);
  hashParam(m_aperture);
  hashParam(m_dof_sp);
}

float ConvertDepthToRadiusOperation::determineFocalDistance()
{
  if (this->m_cameraObject && this->m_cameraObject->type == OB_CAMERA) {
    Camera *camera = (Camera *)this->m_cameraObject->data;
    this->m_cam_lens = camera->lens;
    return BKE_camera_object_dof_distance(this->m_cameraObject);
  }

  return 10.0f;
}

void ConvertDepthToRadiusOperation::initExecution()
{
  float cam_sensor = DEFAULT_SENSOR_WIDTH;
  Camera *camera = NULL;

  if (this->m_cameraObject && this->m_cameraObject->type == OB_CAMERA) {
    camera = (Camera *)this->m_cameraObject->data;
    cam_sensor = BKE_camera_sensor_size(camera->sensor_fit, camera->sensor_x, camera->sensor_y);
  }

  float focalDistance = determineFocalDistance();
  if (focalDistance == 0.0f) {
    focalDistance = 1e10f; /* if the dof is 0.0 then set it to be far away */
  }
  this->m_inverseFocalDistance = 1.0f / focalDistance;
  float aspect = (this->getWidth() > this->getHeight()) ?
                     (this->getHeight() / (float)this->getWidth()) :
                     (this->getWidth() / (float)this->getHeight());
  this->m_aperture = 0.5f * (this->m_cam_lens / (aspect * cam_sensor)) / this->m_fStop;
  const float minsz = CCL::min(getWidth(), getHeight());
  this->m_dof_sp = minsz /
                   ((cam_sensor / 2.0f) /
                    this->m_cam_lens);  // <- == aspect * min(img->x, img->y) / tan(0.5f * fov);

  if (this->m_blurPostOperation) {
    m_blurPostOperation->setSigma(CCL::min(m_aperture * 128.0f, this->m_maxRadius));
  }
  NodeOperation::initExecution();
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel convertDepthToRadiusOp(CCL_WRITE(dst),
                                  CCL_READ(input),
                                  const float aperture,
                                  const float dof_sp,
                                  const float inv_focal_distance,
                                  const float max_radius)
{
  READ_DECL(input);
  WRITE_DECL(dst);
  float radius;
  float z;

  CPU_LOOP_START(dst);

  COPY_COORDS(input, dst_coords);

  READ_IMG1(input, input_pix);
  z = input_pix.x;
  if (z != 0.0f) {
    float iZ = 1.0f / z;

    // bug #6656 part 2b, do not rescale
#if 0
    bcrad = 0.5f * fabs(aperture * (dof_sp * (cam_invfdist - iZ) - 1.0f));
    // scale crad back to original maximum and blend
    crad->rect[px] = bcrad + wts->rect[px] * (scf * crad->rect[px] - bcrad);
#endif
    radius = 0.5f * fabsf(aperture * (dof_sp * (inv_focal_distance - iZ) - 1.0f));
    // 'bug' #6615, limit minimum radius to 1 pixel, not really a solution, but somewhat mitigates
    // the problem
    if (radius < 0.0f) {
      radius = 0.0f;
    }
    if (radius > max_radius) {
      radius = max_radius;
    }
    input_pix.x = radius;
  }
  else {
    input_pix.x = 0.0f;
  }

  WRITE_IMG1(dst, input_pix);

  CPU_LOOP_END
}

CCL_NAMESPACE_END
#undef OPENCL_CODE

void ConvertDepthToRadiusOperation::execPixels(ExecutionManager &man)
{
  auto input = getInputOperation(0)->getPixels(this, man);

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::convertDepthToRadiusOp,
      _1,
      input,
      m_aperture,
      m_dof_sp,
      m_inverseFocalDistance,
      m_maxRadius);
  computeWriteSeek(man, cpu_write, "convertDepthToRadiusOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*input);
    kernel->addFloatArg(m_aperture);
    kernel->addIntArg(m_dof_sp);
    kernel->addIntArg(m_inverseFocalDistance);
    kernel->addIntArg(m_maxRadius);
  });
}
