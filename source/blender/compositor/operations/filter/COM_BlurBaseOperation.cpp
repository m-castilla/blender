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

#include "COM_BlurBaseOperation.h"
#include "BLI_math.h"
#include "COM_GlobalManager.h"
#include "MEM_guardedalloc.h"
#include "RE_pipeline.h"

#include "COM_kernel_cpu.h"

BlurBaseOperation::BlurBaseOperation(SocketType socket_type) : NodeOperation()
{
  /* data_type is almost always COM_DT_COLOR except for alpha-blur */
  this->addInputSocket(socket_type);
  this->addInputSocket(SocketType::VALUE);
  this->addOutputSocket(socket_type);
  memset(&m_data, 0, sizeof(NodeBlurData));
  this->m_size = 1.0f;
  this->m_extend_bounds = false;
}

void BlurBaseOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_size);
  hashParam(m_extend_bounds);
  hashParam(m_data.aspect);
  hashParam(m_data.bokeh);
  hashParam(m_data.curved);
  hashParam(m_data.fac);
  hashParam(m_data.filtertype);
  hashParam(m_data.gamma);
  hashParam(m_data.image_in_height);
  hashParam(m_data.image_in_width);
  hashParam(m_data.maxspeed);
  hashParam(m_data.minspeed);
  hashParam(m_data.percentx);
  hashParam(m_data.percenty);
  hashParam(m_data.relative);
  hashParam(m_data.samples);
  hashParam(m_data.sizex);
  hashParam(m_data.sizey);
}
void BlurBaseOperation::initExecution()
{
  this->m_data.image_in_width = this->getWidth();
  this->m_data.image_in_height = this->getHeight();
  if (this->m_data.relative) {
    int sizex, sizey;
    switch (this->m_data.aspect) {
      case CMP_NODE_BLUR_ASPECT_Y:
        sizex = sizey = this->m_data.image_in_width;
        break;
      case CMP_NODE_BLUR_ASPECT_X:
        sizex = sizey = this->m_data.image_in_height;
        break;
      default:
        BLI_assert(this->m_data.aspect == CMP_NODE_BLUR_ASPECT_NONE);
        sizex = this->m_data.image_in_width;
        sizey = this->m_data.image_in_height;
        break;
    }
    this->m_data.sizex = round_fl_to_int(this->m_data.percentx * 0.01f * sizex);
    this->m_data.sizey = round_fl_to_int(this->m_data.percenty * 0.01f * sizey);
  }

  QualityStepHelper::initExecution(QualityHelper::MULTIPLY);
  NodeOperation::initExecution();
}

int BlurBaseOperation::make_gausstab(float *buffer, int buffer_start, float rad, int size)
{
  int n_elems = get_gausstab_n_elems(size);

  float sum = 0.0f;
  float fac = (rad > 0.0f ? 1.0f / rad : 0.0f);
  float val;
  for (int i = -size; i <= size; i++) {
    val = RE_filter_value(this->m_data.filtertype, (float)i * fac);
    sum += val;
    buffer[buffer_start + i + size] = val;
  }

  sum = 1.0f / sum;
  for (int i = 0; i < n_elems; i++) {
    buffer[buffer_start + i] = buffer[buffer_start + i] * sum;
  }

  return n_elems;
}

TmpBuffer *BlurBaseOperation::make_gausstab(float rad, int size)
{
  int n_elems = get_gausstab_n_elems(size);
  BufferRecycler *recycler = GlobalMan->BufferMan->recycler();
  TmpBuffer *buf = recycler->createTmpBuffer();
  recycler->takeRecycle(BufferRecycleType::HOST_CLEAR, buf, n_elems, 1, 1);

  int gauss_n_elems = make_gausstab(buf->host.buffer, 0, rad, size);
  BLI_assert(n_elems == gauss_n_elems);
  (void)gauss_n_elems;

  return buf;
}

/* normalized distance from the current (inverted so 1.0 is close and 0.0 is far)
 * 'ease' is applied after, looks nicer */
TmpBuffer *BlurBaseOperation::make_dist_fac_inverse(float rad, int size, int falloff)
{
  float val;
  int i, n_elems;

  n_elems = 2 * size + 1;
  BufferRecycler *recycler = GlobalMan->BufferMan->recycler();
  TmpBuffer *tmp_buf = recycler->createTmpBuffer();
  recycler->takeRecycle(BufferRecycleType::HOST_CLEAR, tmp_buf, n_elems, 1, 1);

  float *buffer = tmp_buf->host.buffer;

  float fac = (rad > 0.0f ? 1.0f / rad : 0.0f);
  for (i = -size; i <= size; i++) {
    val = 1.0f - fabsf((float)i * fac);

    /* keep in sync with rna_enum_proportional_falloff_curve_only_items */
    switch (falloff) {
      case PROP_SMOOTH:
        /* ease - gives less hard lines for dilate/erode feather */
        val = (3.0f * val * val - 2.0f * val * val * val);
        break;
      case PROP_SPHERE:
        val = sqrtf(2.0f * val - val * val);
        break;
      case PROP_ROOT:
        val = sqrtf(val);
        break;
      case PROP_SHARP:
        val = val * val;
        break;
      case PROP_INVSQUARE:
        val = val * (2.0f - val);
        break;
      case PROP_LIN:
        /* nothing to do */
        break;
#ifndef NDEBUG
      case -1:
        /* uninitialized! */
        BLI_assert(0);
        break;
#endif
      default:
        /* nothing */
        break;
    }
    buffer[i + size] = val;
  }

  return tmp_buf;
}

void BlurBaseOperation::setData(const NodeBlurData *data)
{
  memcpy(&m_data, data, sizeof(NodeBlurData));
}

ResolutionType BlurBaseOperation::determineResolution(int resolution[2],
                                                      int preferredResolution[2],
                                                      bool setResolution)
{
  NodeOperation::determineResolution(resolution, preferredResolution, setResolution);
  if (this->m_extend_bounds) {
    resolution[0] += 2 * this->m_size * m_data.sizex;
    resolution[1] += 2 * this->m_size * m_data.sizey;
  }
  return ResolutionType::Determined;
}
