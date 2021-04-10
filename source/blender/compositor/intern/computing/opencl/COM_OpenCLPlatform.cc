#include "COM_OpenCLPlatform.h"
#include "BLI_assert.h"
#include "COM_OpenCLKernel.h"
#include "COM_OpenCLManager.h"

namespace blender::compositor {
/* CL_RGB/CL_FLOAT and CL_R/CL_FLOAT not supported in most implementations, we have to use CL_RGBA
 */
const cl_image_format IMAGE_FORMAT_COLOR = {
    CL_RGBA,
    CL_FLOAT,
};
OpenCLPlatform::OpenCLPlatform(OpenCLManager &man, cl_context context, cl_program program)
    : m_context(context), m_program(program), m_man(man), m_samplers()
{
}

OpenCLPlatform::~OpenCLPlatform()
{
  for (auto sampler : m_samplers.values()) {
    m_man.printIfError(clReleaseSampler((cl_sampler)sampler));
  }
  if (m_program) {
    m_man.printIfError(clReleaseProgram(m_program));
  }
  if (m_context) {
    m_man.printIfError(clReleaseContext(m_context));
  }
}

const cl_image_format *OpenCLPlatform::getImageFormat() const
{
  // Only supported 4 channels due to the only FLOAT format that is assured to be compatible in all
  // devices is CL_RGBA/FLOAT
  return &IMAGE_FORMAT_COLOR;
}

int OpenCLPlatform::getMemoryAccessFlag(ComputeAccess mem_access) const
{
  switch (mem_access) {
    case ComputeAccess::READ:
      return CL_MEM_READ_ONLY;
    case ComputeAccess::WRITE:
      return CL_MEM_WRITE_ONLY;
    case ComputeAccess::READ_WRITE:
      return CL_MEM_READ_WRITE;
    default:
      BLI_assert(!"Non implemented ComputeAccess case");
      break;
  }
  return 0;
}

ComputeKernel *OpenCLPlatform::createKernel(const blender::StringRef kernel_name,
                                            ComputeDevice *device)
{
  cl_int error;
  cl_kernel cl_kernel = clCreateKernel(m_program, kernel_name.data(), &error);
  m_man.printIfError(error);

  auto kernel = new OpenCLKernel(m_man, *this, (OpenCLDevice *)device, kernel_name, cl_kernel);
  kernel->initialize();
  return (ComputeKernel *)kernel;
}

cl_sampler OpenCLPlatform::createSampler(ComputeInterpolation interp, ComputeExtend extend)
{
  cl_int error;
  cl_addressing_mode address;
  cl_filter_mode filter;
  switch (extend) {
    case ComputeExtend::UNCHECKED:
      address = CL_ADDRESS_NONE;
      break;
    case ComputeExtend::CLIP:
      address = CL_ADDRESS_CLAMP;
      break;
    case ComputeExtend::EXTEND:
      address = CL_ADDRESS_CLAMP_TO_EDGE;
      break;
    case ComputeExtend::REPEAT:
      address = CL_ADDRESS_REPEAT;
      break;
    case ComputeExtend::MIRROR:
      address = CL_ADDRESS_MIRRORED_REPEAT;
      break;
    default:
      address = CL_ADDRESS_NONE;
      BLI_assert(!"Non implemented PixelExtend case");
  }
  switch (interp) {
    case ComputeInterpolation::NEAREST:
      filter = CL_FILTER_NEAREST;
      break;
    case ComputeInterpolation::BILINEAR:
      filter = CL_FILTER_LINEAR;
      break;
    default:
      filter = CL_FILTER_NEAREST;
      BLI_assert(!"Non implemented PixelInterpolation case");
  }

  bool norm_coords = extend == ComputeExtend::REPEAT || extend == ComputeExtend::MIRROR;
  cl_sampler sampler = clCreateSampler(
      m_context, norm_coords ? CL_TRUE : CL_FALSE, address, filter, &error);
  m_man.printIfError(error);
  return sampler;
}

cl_sampler OpenCLPlatform::getSampler(ComputeInterpolation interp, ComputeExtend extend)
{
  auto key = std::make_pair((int)interp, (int)extend);
  if (m_samplers.contains(key)) {
    return m_samplers.lookup(key);
  }
  else {
    auto sampler = createSampler(interp, extend);
    m_samplers.add(key, sampler);
    return sampler;
  }
}

}  // namespace blender::compositor
