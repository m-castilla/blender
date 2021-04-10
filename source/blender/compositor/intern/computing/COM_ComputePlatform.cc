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
 * Copyright 2021, Blender Foundation.
 */

#include "COM_ComputePlatform.h"
#include "BLI_assert.h"
#include "COM_ComputeKernel.h"

namespace blender::compositor {

const int MAX_KERNELS_IN_CACHE = 1000;

ComputePlatform::ComputePlatform() : m_kernels(), m_kernels_count(0)
{
}

ComputePlatform::~ComputePlatform()
{
  cleanKernelsCache();
}

ComputeKernelUniquePtr ComputePlatform::takeKernel(const blender::StringRef kernel_name,
                                                   ComputeDevice *device)
{
  ComputeKernel *kernel = nullptr;
  auto &kernels = m_kernels.lookup(kernel_name);
  if (!kernels.is_empty()) {
    kernel = kernels.pop_last();
    m_kernels_count--;
  }

  if (!kernel) {
    kernel = createKernel(kernel_name, device);
  }
  else {
    kernel->reset(device);
  }

  auto deleter = [this](ComputeKernel *kernel) { recycleKernel(kernel); };
  return ComputeKernelUniquePtr(kernel, deleter);
}

void ComputePlatform::recycleKernel(ComputeKernel *kernel)
{
  kernel->clearArgs();
  auto kernel_name = kernel->getKernelName();

  if (m_kernels_count >= MAX_KERNELS_IN_CACHE) {
    cleanKernelsCache();
  }
  if (m_kernels.contains(kernel_name)) {
    m_kernels.lookup(kernel_name).append(kernel);
  }
  else {
    blender::Vector<ComputeKernel *> kernels = {};
    kernels.append(kernel);
    m_kernels.add(kernel_name, std::move(kernels));
  }
  m_kernels_count++;
}

void ComputePlatform::cleanKernelsCache()
{
  for (auto &kernels : m_kernels.values()) {
    for (auto kernel : kernels) {
      delete kernel;
    }
  }
  m_kernels.clear();
  m_kernels_count = 0;
}

}  // namespace blender::compositor
