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
 * Copyright 2012, Blender Foundation.
 */

#pragma once

#include "COM_NodeOperation.h"
#include <mutex>
#include <string.h>

struct MovieClip;
struct VoronoiTriangulationPoint;
/**
 * Class with implementation of green screen gradient rasterization
 */
class KeyingScreenOperation : public NodeOperation {
 private:
  std::mutex mutex;

 protected:
  typedef struct TriangulationData {
    VoronoiTriangulationPoint *triangulated_points;
    int (*triangles)[3];
    int triangulated_points_total, triangles_total;
    rcti *triangles_AABB;
  } TriangulationData;

  typedef struct RectTriangles {
    int *triangles;
    int triangles_total;
  } RectTriangles;

  MovieClip *m_movieClip;
  int m_framenumber;
  TriangulationData *m_triangulation;
  char m_trackingObject[64];

  TriangulationData *buildVoronoiTriangulation();

 public:
  KeyingScreenOperation();

  void deinitExecution();

  RectTriangles *createTriangles(PixelsRect *dst_rect);
  void deleteTriangles(RectTriangles *rect_triangles);

  void setMovieClip(MovieClip *clip)
  {
    this->m_movieClip = clip;
  }

  void setTrackingObject(const char *object);

  void setFramenumber(int framenumber)
  {
    this->m_framenumber = framenumber;
  }

  virtual ResolutionType determineResolution(int resolution[2],
                                             int preferredResolution[2],
                                             bool setResolution) override;

 protected:
  virtual bool canCompute() const override
  {
    return false;
  }
  virtual void hashParams() override;
  virtual void execPixels(ExecutionManager &man) override;
};
