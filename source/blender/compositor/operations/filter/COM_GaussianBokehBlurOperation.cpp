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

#include "COM_GaussianBokehBlurOperation.h"
#include "BLI_utildefines.h"
#include "COM_Buffer.h"
#include "COM_ComputeKernel.h"
#include "COM_ExecutionManager.h"
#include "COM_GlobalManager.h"
#include "COM_kernel_cpu.h"
#include "MEM_guardedalloc.h"
#include "RE_pipeline.h"

using namespace std::placeholders;
GaussianBokehBlurOperation::GaussianBokehBlurOperation() : BlurBaseOperation(SocketType::COLOR)
{
  m_radx = 0.0f;
  m_rady = 0.0f;
}

TmpBuffer *GaussianBokehBlurOperation::calc_bokeh_gausstab()
{
  float radxf;
  float radyf;
  int n;
  float *dgauss;
  float *ddgauss;
  int j, i;
  const float width = this->getWidth();
  const float height = this->getHeight();

  radxf = m_size * (float)this->m_data.sizex;
  CLAMP(radxf, 0.0f, width / 2.0f);

  /* vertical */
  radyf = this->m_size * (float)this->m_data.sizey;
  CLAMP(radyf, 0.0f, height / 2.0f);

  this->m_radx = ceil(radxf);
  this->m_rady = ceil(radyf);

  int ddwidth = 2 * this->m_radx + 1;
  int ddheight = 2 * this->m_rady + 1;
  n = ddwidth * ddheight;

  auto recycler = GlobalMan->BufferMan->recycler();
  auto tmp_buf = recycler->createTmpBuffer();
  recycler->takeNonStdRecycle(tmp_buf, n, 1, 1);

  /* create a full filter image */
  ddgauss = tmp_buf->host.buffer;
  dgauss = ddgauss;
  float sum = 0.0f;
  float facx = (radxf > 0.0f ? 1.0f / radxf : 0.0f);
  float facy = (radyf > 0.0f ? 1.0f / radyf : 0.0f);
  for (j = -this->m_rady; j <= this->m_rady; j++) {
    for (i = -this->m_radx; i <= this->m_radx; i++, dgauss++) {
      float fj = (float)j * facy;
      float fi = (float)i * facx;
      float dist = sqrt(fj * fj + fi * fi);
      *dgauss = RE_filter_value(this->m_data.filtertype, dist);

      sum += *dgauss;
    }
  }

  if (sum > 0.0f) {
    /* normalize */
    float norm = 1.0f / sum;
    for (j = n - 1; j >= 0; j--) {
      ddgauss[j] *= norm;
    }
  }
  else {
    int center = m_rady * ddwidth + m_radx;
    ddgauss[center] = 1.0f;
  }

  return tmp_buf;
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel gaussianBokehBlurOp(CCL_WRITE(dst),
                               CCL_READ(color),
                               ccl_constant float *gausstab,
                               int width,
                               int height,
                               int radx,
                               int rady,
                               int quality_step)
{
  READ_DECL(color);
  WRITE_DECL(dst);

  CPU_LOOP_START(dst);

  int xmin = max(dst_coords.x - radx, 0);
  int xmax = min(dst_coords.x + rady + 1, width);
  int ymin = max(dst_coords.y - rady, 0);
  int ymax = min(dst_coords.y + rady + 1, height);

  /* gauss */
  const int addConst = (xmin - dst_coords.x + radx);
  const int mulConst = (radx * 2 + 1);
  float4 color_accum = make_float4_1(0.0f);
  float multiplier_accum = 0.0f;

  SET_COORDS(color, xmin, ymin);
  while (color_coords.y < ymax) {
    int index = ((color_coords.y - dst_coords.y) + rady) * mulConst + addConst;
    while (color_coords.x < xmax) {
      const float4 multiplier = make_float4_1(gausstab[index]);
      READ_IMG4(color, color_pix);
      color_accum += color_pix * multiplier;
      multiplier_accum += multiplier.x;
      index += quality_step;
      INCR_COORDS_X(color, quality_step);
    }
    INCR_COORDS_Y(color, quality_step);
    UPDATE_COORDS_X(color, xmin);
  }

  color_pix = color_accum * (1.0f / multiplier_accum);

  WRITE_IMG4(dst, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void GaussianBokehBlurOperation::execPixels(ExecutionManager &man)
{
  auto value = getInputOperation(0)->getPixels(this, man);

  int quality_step = QualityStepHelper::getStep();
  TmpBuffer *gausstab = nullptr;
  if (man.canExecPixels()) {
    gausstab = GaussianBokehBlurOperation::calc_bokeh_gausstab();
  }

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::gaussianBokehBlurOp,
      _1,
      value,
      gausstab ? gausstab->host.buffer : nullptr,
      getWidth(),
      getHeight(),
      m_radx,
      m_rady,
      quality_step);
  computeWriteSeek(man, cpu_write, "gaussianBokehBlurOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*value);
    kernel->addFloatCArrayArg(gausstab->host.buffer, gausstab->width * gausstab->height);
    kernel->addIntArg(getWidth());
    kernel->addIntArg(getHeight());
    kernel->addIntArg(m_radx);
    kernel->addIntArg(m_rady);
    kernel->addIntArg(quality_step);
  });

  if (gausstab) {
    GlobalMan->BufferMan->recycler()->giveRecycle(gausstab);
  }
}

// reference image
GaussianBlurReferenceOperation::GaussianBlurReferenceOperation()
    : BlurBaseOperation(SocketType::COLOR),
      m_filtersizex(0),
      m_filtersizey(0),
      m_radx(0),
      m_rady(0)
{
}

void GaussianBlurReferenceOperation::initExecution()
{
  BlurBaseOperation::initExecution();

  // setup gaustab
  this->m_data.image_in_width = this->getWidth();
  this->m_data.image_in_height = this->getHeight();
  if (this->m_data.relative) {
    switch (this->m_data.aspect) {
      case CMP_NODE_BLUR_ASPECT_NONE:
        this->m_data.sizex = (int)(this->m_data.percentx * 0.01f * this->m_data.image_in_width);
        this->m_data.sizey = (int)(this->m_data.percenty * 0.01f * this->m_data.image_in_height);
        break;
      case CMP_NODE_BLUR_ASPECT_Y:
        this->m_data.sizex = (int)(this->m_data.percentx * 0.01f * this->m_data.image_in_width);
        this->m_data.sizey = (int)(this->m_data.percenty * 0.01f * this->m_data.image_in_width);
        break;
      case CMP_NODE_BLUR_ASPECT_X:
        this->m_data.sizex = (int)(this->m_data.percentx * 0.01f * this->m_data.image_in_height);
        this->m_data.sizey = (int)(this->m_data.percenty * 0.01f * this->m_data.image_in_height);
        break;
    }
  }

  /* horizontal */
  m_filtersizex = (float)this->m_data.sizex;
  int imgx = getWidth() / 2;
  if (m_filtersizex > imgx) {
    m_filtersizex = imgx;
  }
  else if (m_filtersizex < 1) {
    m_filtersizex = 1;
  }
  m_radx = (float)m_filtersizex;

  /* vertical */
  m_filtersizey = (float)this->m_data.sizey;
  int imgy = getHeight() / 2;
  if (m_filtersizey > imgy) {
    m_filtersizey = imgy;
  }
  else if (m_filtersizey < 1) {
    m_filtersizey = 1;
  }
  m_rady = (float)m_filtersizey;
}

std::tuple<TmpBuffer *, int *, int> GaussianBlurReferenceOperation::make_ref_gauss()
{
  int n_tabs = CCL::max(m_filtersizex, m_filtersizey);
  int *tabs_offsets = (int *)MEM_mallocN(n_tabs * sizeof(int), __func__);

  int total_elems = 0;
  for (int i = 0; i < n_tabs; i++) {
    tabs_offsets[i] = total_elems;
    int n_elems = get_gausstab_n_elems(i + 1);
    total_elems += n_elems;
  }

  auto recycler = GlobalMan->BufferMan->recycler();
  auto tmp_buf = recycler->createTmpBuffer();
  recycler->takeNonStdRecycle(tmp_buf, total_elems, 1, 1);

  float *gausstab = tmp_buf->host.buffer;
  for (int i = 0; i < n_tabs; i++) {
    BlurBaseOperation::make_gausstab(gausstab, tabs_offsets[i], i + 1, i + 1);
  }

  return std::make_tuple(tmp_buf, tabs_offsets, n_tabs);
}

#define OPENCL_CODE
CCL_NAMESPACE_BEGIN
ccl_kernel gaussianBlurReferenceOp(CCL_WRITE(dst),
                                   CCL_READ(color),
                                   CCL_READ(size),
                                   ccl_constant float *gausstabs,
                                   ccl_constant int *gausstabs_offsets,
                                   int width,
                                   int height,
                                   float rad_x,
                                   float rad_y,
                                   int filtersize_x,
                                   int filtersize_y)
{
  READ_DECL(color);
  READ_DECL(size);
  WRITE_DECL(dst);

  ccl_constant float *gausstabx, *gausstabcenty;
  ccl_constant float *gausstaby, *gausstabcentx;

  CPU_LOOP_START(dst);

  COPY_COORDS(size, dst_coords);
  READ_IMG1(size, size_pix);

  float refSize = size_pix.x;
  int refradx = (int)(refSize * rad_x);
  int refrady = (int)(refSize * rad_y);
  if (refradx > filtersize_x) {
    refradx = filtersize_x;
  }
  else if (refradx < 1) {
    refradx = 1;
  }
  if (refrady > filtersize_y) {
    refrady = filtersize_y;
  }
  else if (refrady < 1) {
    refrady = 1;
  }

  if (refradx == 1 && refrady == 1) {
    COPY_COORDS(color, dst_coords);
    READ_IMG4(color, color_pix);
  }
  else {
    int minxr = dst_coords.x - refradx < 0 ? -dst_coords.x : -refradx;
    int maxxr = dst_coords.x + refradx > width ? width - dst_coords.x : refradx;
    int minyr = dst_coords.y - refrady < 0 ? -dst_coords.y : -refrady;
    int maxyr = dst_coords.y + refrady > height ? height - dst_coords.y : refrady;

    int gausstabx_offset = gausstabs_offsets[(refradx - 1)];
    gausstabx = gausstabs + (size_t)gausstabx_offset;
    gausstabcentx = gausstabx + refradx;

    int gausstaby_offset = gausstabs_offsets[(refrady - 1)];
    gausstaby = gausstabs + (size_t)gausstaby_offset;
    gausstabcenty = gausstaby + refrady;

    float4 color_accum = make_float4_1(0.0f);
    float val = 0.0f;
    float sum = 0.0f;

    int color_start_y = dst_coords.y + minyr;
    int color_start_x = dst_coords.x + minxr;
    SET_COORDS(color, color_start_x, color_start_y);
    for (int i = minyr; i < maxyr; i++) {
      for (int j = minxr; j < maxxr; j++) {
        val = gausstabcenty[i] * gausstabcentx[j];
        sum += val;
        READ_IMG4(color, color_pix);
        color_accum += color_pix * val;
        INCR1_COORDS_X(color);
      }
      INCR1_COORDS_Y(color);
      UPDATE_COORDS_X(color, color_start_x);
    }

    color_pix = color_accum * (1.0f / sum);
  }

  WRITE_IMG4(dst, color_pix);

  CPU_LOOP_END
}
CCL_NAMESPACE_END
#undef OPENCL_CODE

void GaussianBlurReferenceOperation::execPixels(ExecutionManager &man)
{
  auto color = getInputOperation(0)->getPixels(this, man);
  auto size = getInputOperation(1)->getPixels(this, man);

  TmpBuffer *ref_gauss = nullptr;
  int *ref_gauss_offsets = nullptr;
  int n_gauss_tabs = 0;
  if (man.canExecPixels()) {
    auto gauss_data = make_ref_gauss();
    ref_gauss = std::get<0>(gauss_data);
    ref_gauss_offsets = std::get<1>(gauss_data);
    n_gauss_tabs = std::get<2>(gauss_data);
  }

  std::function<void(PixelsRect &, const WriteRectContext &)> cpu_write = std::bind(
      CCL::gaussianBlurReferenceOp,
      _1,
      color,
      size,
      ref_gauss ? ref_gauss->host.buffer : nullptr,
      ref_gauss_offsets,
      getWidth(),
      getHeight(),
      m_radx,
      m_rady,
      m_filtersizex,
      m_filtersizey);
  computeWriteSeek(man, cpu_write, "gaussianBlurReferenceOp", [&](ComputeKernel *kernel) {
    kernel->addReadImgArgs(*color);
    kernel->addReadImgArgs(*size);
    kernel->addFloatCArrayArg(ref_gauss->host.buffer, ref_gauss->width * ref_gauss->height);
    kernel->addIntCArrayArg(ref_gauss_offsets, n_gauss_tabs);
    kernel->addIntArg(getWidth());
    kernel->addIntArg(getHeight());
    kernel->addFloatArg(m_radx);
    kernel->addFloatArg(m_rady);
    kernel->addIntArg(m_filtersizex);
    kernel->addIntArg(m_filtersizey);
  });

  if (ref_gauss) {
    GlobalMan->BufferMan->recycler()->giveRecycle(ref_gauss);
  }
  if (ref_gauss_offsets) {
    MEM_freeN(ref_gauss_offsets);
  }
}
