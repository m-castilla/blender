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

#include "COM_ComputeManager.h"
#include "BKE_appdir.h"
#include "BLI_assert.h"
#include "BLI_math_color.h"
#include "BLI_path_util.h"
#include "BLI_rect.h"
#include "COM_ComputeDevice.h"
#include "COM_ExecutionManager.h"
#include "COM_ExecutionSystem.h"
#include "COM_Pixels.h"
#include "COM_PixelsUtil.h"
#include "COM_defines.h"
#include "COM_kernel_cpu.h"
#include "DNA_key_types.h"
#include "DNA_texture_types.h"
#include "MEM_guardedalloc.h"
#include <string.h>

#if defined(COM_DEBUG) || defined(DEBUG)

#  include "COM_DefMerger.h"
#else
#  include "COM_OpenCLKernels.cl.h"
#endif

/* Pass arguments into this contructor*/
ComputeManager::ComputeManager() : m_device(nullptr), m_is_initialized(false)
{
}
ComputeManager::~ComputeManager()
{
}

void ComputeManager::initialize()
{
  if (!m_is_initialized) {
    assertKernelsSync();
    auto devices = getDevices();
    auto selected_compute_units = 0;
    for (auto dev : devices) {
      dev->initialize();
      if (dev->getDeviceType() == ComputeDeviceType::GPU) {
        if (selected_compute_units < dev->getNComputeUnits()) {
          selected_compute_units = dev->getNComputeUnits();
          m_device = dev;
        }
      }
    }

    m_is_initialized = true;
  }
}

void ComputeManager::assertKernelsSync()
{
  BLI_assert(CCL_YCC_ITU_BT601 == BLI_YCC_ITU_BT601);
  BLI_assert(CCL_YCC_ITU_BT709 == CCL_YCC_ITU_BT709);
  BLI_assert(CCL_YCC_JFIF_0_255 == BLI_YCC_JFIF_0_255);

  BLI_assert(CCL_MASK_TYPE_ADD == CMP_NODE_MASKTYPE_ADD);
  BLI_assert(CCL_MASK_TYPE_SUBTRACT == CMP_NODE_MASKTYPE_SUBTRACT);
  BLI_assert(CCL_MASK_TYPE_MULTIPLY == CMP_NODE_MASKTYPE_MULTIPLY);
  BLI_assert(CCL_MASK_TYPE_NOT == CMP_NODE_MASKTYPE_NOT);

  BLI_assert(CCL_COLBAND_INTERP_LINEAR == COLBAND_INTERP_LINEAR);
  BLI_assert(CCL_COLBAND_INTERP_EASE == COLBAND_INTERP_EASE);
  BLI_assert(CCL_COLBAND_INTERP_B_SPLINE == COLBAND_INTERP_B_SPLINE);
  BLI_assert(CCL_COLBAND_INTERP_CARDINAL == COLBAND_INTERP_CARDINAL);
  BLI_assert(CCL_COLBAND_INTERP_CONSTANT == COLBAND_INTERP_CONSTANT);

  BLI_assert(CCL_COLBAND_HUE_NEAR == COLBAND_HUE_NEAR);
  BLI_assert(CCL_COLBAND_HUE_FAR == COLBAND_HUE_FAR);
  BLI_assert(CCL_COLBAND_HUE_CW == COLBAND_HUE_CW);
  BLI_assert(CCL_COLBAND_HUE_CCW == COLBAND_HUE_CCW);

  BLI_assert(CCL_TEXMAP_CLIP_MIN == TEXMAP_CLIP_MIN);
  BLI_assert(CCL_TEXMAP_CLIP_MAX == TEXMAP_CLIP_MAX);

  BLI_assert(CCL_KEY_LINEAR == KEY_LINEAR);
  BLI_assert(CCL_KEY_CARDINAL == KEY_CARDINAL);
  BLI_assert(CCL_KEY_BSPLINE == KEY_BSPLINE);
  BLI_assert(CCL_KEY_CATMULL_ROM == KEY_CATMULL_ROM);
}

int ComputeManager::getNComputeUnits()
{
  return m_device == nullptr ? 0 : m_device->getNComputeUnits();
}

int ComputeManager::getMaxImgH()
{
  return m_device == nullptr ? 0 : m_device->getMaxImgH();
}

int ComputeManager::getMaxImgW()
{
  return m_device == nullptr ? 0 : m_device->getMaxImgW();
}

std::pair<std::string, std::string> ComputeManager::loadKernelsSource()
{
#if defined(COM_DEBUG) || defined(DEBUG)
  /* get dst path and kernel code tag */
  std::string kernels_filename, code_tag, include_filename;
  switch (getComputeType()) {
    case ComputeType::OPENCL:
      /* get dst path and kernel code tag */
      kernels_filename = "COM_OpenCLKernels.cl";
      code_tag = "OPENCL_CODE";
      include_filename = "COM_kernel_opencl.h";
      break;
    default:
      BLI_assert(!"Non implemented case for compositor compute type");
  }
  auto app_dir = BKE_appdir_program_dir();
  char *blender_main = strdup(app_dir);
  std::vector<std::string> compo_defmerged_paths = {"operations/"};

  /* find bin folder */
  int r_offset, r_len;
  bool bin_found = false;
  bool path_end_reached = false;
  int bin_folder_parent_idx = 0;
  while (!bin_found && !path_end_reached) {
    bin_folder_parent_idx--;
    path_end_reached = !BLI_path_name_at_index(app_dir, bin_folder_parent_idx, &r_offset, &r_len);
    bin_found = !path_end_reached && r_len == 3 && app_dir[r_offset] == 'b' &&
                app_dir[r_offset + 1] == 'i' && app_dir[r_offset + 2] == 'n';
  }
  BLI_assert(!path_end_reached);

  /*get compositor computing directory path*/
  bin_folder_parent_idx--;  // once more to get to blender main dir
  auto p = BLI_path_name_at_index(app_dir, bin_folder_parent_idx, &r_offset, &r_len);
  UNUSED_VARS(p);
  blender_main[r_offset] = '\0';
  std::string source_dst = std::string(blender_main);
  free(blender_main);
  source_dst.append("blender/source/blender/compositor/");
  std::string compo_path = std::string(source_dst);
  source_dst = compo_path + "computing/";
  source_dst.append(kernels_filename);

  /* get src paths */
  std::vector<char *> defmerged_paths;
  for (auto relpath : compo_defmerged_paths) {
    defmerged_paths.push_back(strdup((compo_path + relpath).c_str()));
  }

  /* merge all kernel source code from determined paths */
  auto result = DefMerger::defmerge(code_tag.c_str(),
                                    source_dst.c_str(),
                                    include_filename.c_str(),
                                    defmerged_paths.size(),
                                    (const char **)&defmerged_paths[0],
                                    false);
  for (auto path : defmerged_paths) {
    free(path);
  }

  return {result, source_dst};
#else
  switch (getComputeType()) {
    case ComputeType::OPENCL:
      return {datatoc_COM_OpenCLKernels_cl, ""};
    default:
      BLI_assert(!"Non implemented compositor compute type");
      return {"", ""};
  };
#endif
}
