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
 * Copyright 2020, Blender Foundation.
 */

#include "COM_ComputePlatform.h"
#include "BLI_assert.h"
#include "COM_ComputeKernel.h"
#include "COM_Pixels.h"
const int MAX_KERNELS_IN_CACHE = 1000;

ComputePlatform::ComputePlatform()
    : m_kernels(), m_kernels_count(0), m_is_initialized(false), m_samplers_deleted(false)
{
}

ComputePlatform::~ComputePlatform()
{
  cleanKernelsCache();
  if (!m_samplers_deleted) {
    BLI_assert(!"ComputePlatform is being destroyed but deleteSamplers has not been called. It must be called in subclasses destructors");
  }
}

void ComputePlatform::initialize()
{
  PixelInterpolationForeach([&](PixelInterpolation interp) {
    PixelExtendForeach([&](PixelExtend extend) {
      PixelsSampler pix_sampler = {interp, extend};
      m_samplers.insert({pix_sampler, createSampler(pix_sampler)});
    });
  });
}

void ComputePlatform::deleteSamplers(std::function<void(void *)> deleteFunc)
{
  for (auto entry : m_samplers) {
    auto sampler = entry.second;
    deleteFunc(sampler);
  }
  m_samplers_deleted = true;
}

void *ComputePlatform::getSampler(PixelsSampler &pix_sampler)
{
  BLI_assert(m_samplers.find(pix_sampler) != m_samplers.end());
  return m_samplers[pix_sampler];
}

/* recycleKernel must be called after finished using the kernel */
ComputeKernel *ComputePlatform::getKernel(std::string kernel_name, ComputeDevice *device)
{
  ComputeKernel *kernel = nullptr;

  auto find_it = m_kernels.find(kernel_name);
  if (find_it != m_kernels.end()) {
    std::vector<ComputeKernel *> &kernels = find_it->second;
    if (kernels.size() > 0) {
      kernel = kernels.back();
      kernels.pop_back();
      m_kernels_count--;
    }
  }

  if (!kernel) {
    kernel = createKernel(kernel_name, device);
  }
  else {
    kernel->reset(device);
  }

  return kernel;
}

void ComputePlatform::recycleKernel(ComputeKernel *kernel)
{
  auto kernel_name = kernel->getKernelName();

  if (m_kernels_count >= MAX_KERNELS_IN_CACHE) {
    cleanKernelsCache();
  }
  auto find_it = m_kernels.find(kernel_name);
  if (find_it == m_kernels.end()) {
    std::vector<ComputeKernel *> kernels = {};
    kernels.push_back(kernel);
    m_kernels.insert({kernel_name, std::move(kernels)});
  }
  else {
    find_it->second.push_back(kernel);
  }
  m_kernels_count++;
}

void ComputePlatform::cleanKernelsCache()
{
  std::vector<ComputeKernel *> to_delete(m_kernels_count);
  for (auto map_entry : m_kernels) {
    std::vector<ComputeKernel *> &kernels = map_entry.second;
    if (kernels.size() > 0) {
      to_delete.insert(to_delete.end(), kernels.begin(), kernels.end());
    }
  }
  m_kernels.clear();
  m_kernels_count = 0;

  for (auto kernel : to_delete) {
    delete kernel;
  }
}
