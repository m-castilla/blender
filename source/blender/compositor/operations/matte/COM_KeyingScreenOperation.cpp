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

#include <stddef.h>

#include "BKE_movieclip.h"
#include "BKE_tracking.h"
#include "BLI_listbase.h"
#include "BLI_math.h"
#include "BLI_math_color.h"
#include "BLI_string.h"
#include "BLI_voronoi_2d.h"
#include "DNA_movieclip_types.h"
#include "IMB_imbuf.h"
#include "IMB_imbuf_types.h"
#include "MEM_guardedalloc.h"

#include "COM_KeyingScreenOperation.h"

#include "COM_kernel_cpu.h"

KeyingScreenOperation::KeyingScreenOperation() : NodeOperation()
{
  this->addOutputSocket(SocketType::COLOR);
  this->m_movieClip = NULL;
  this->m_framenumber = 0;
  this->m_trackingObject[0] = 0;
  m_triangulation = nullptr;
}

void KeyingScreenOperation::hashParams()
{
  NodeOperation::hashParams();
  hashParam(m_framenumber);
  if (m_movieClip) {
    hashParam(m_movieClip->id.session_uuid);
    /* hash tracking */
    MovieTracking *tracking = &this->m_movieClip->tracking;
    ListBase *tracksbase = nullptr;
    if (this->m_trackingObject[0]) {
      MovieTrackingObject *object = BKE_tracking_object_get_named(tracking,
                                                                  this->m_trackingObject);

      if (object) {
        tracksbase = BKE_tracking_object_get_tracks(tracking, object);
      }
    }
    else {
      tracksbase = BKE_tracking_get_active_tracks(tracking);
    }

    if (tracksbase) {
      int clip_frame = BKE_movieclip_remap_scene_to_clip_frame(this->m_movieClip,
                                                               this->m_framenumber);
      for (MovieTrackingTrack *track = (MovieTrackingTrack *)tracksbase->first; track;
           track = track->next) {
        MovieTrackingMarker *marker = BKE_tracking_marker_get(track, clip_frame);
        hashParam(marker->flag);
        /* Not needed as BKE_tracking_marker_get already takes this into account to return the
         * appropiate marker */
        // hashParam(marker->framenr);
        hashFloatData(marker->pos, 2);
        hashFloatData(marker->search_max, 2);
        hashFloatData(marker->search_min, 2);
        hashFloatData(marker->pattern_corners[0], 2);
        hashFloatData(marker->pattern_corners[1], 2);
        hashFloatData(marker->pattern_corners[2], 2);
        hashFloatData(marker->pattern_corners[3], 2);
      }
    }
    /* */
  }
}

void KeyingScreenOperation::setTrackingObject(const char *object)
{
  BLI_strncpy(this->m_trackingObject, object, sizeof(this->m_trackingObject));
}

void KeyingScreenOperation::deinitExecution()
{
  if (this->m_triangulation) {
    TriangulationData *triangulation = this->m_triangulation;

    if (triangulation->triangulated_points) {
      MEM_freeN(triangulation->triangulated_points);
    }

    if (triangulation->triangles) {
      MEM_freeN(triangulation->triangles);
    }

    if (triangulation->triangles_AABB) {
      MEM_freeN(triangulation->triangles_AABB);
    }

    MEM_freeN(this->m_triangulation);

    this->m_triangulation = NULL;
  }
}

KeyingScreenOperation::TriangulationData *KeyingScreenOperation::buildVoronoiTriangulation()
{
  MovieClipUser user = {0};
  TriangulationData *triangulation;
  MovieTracking *tracking = &this->m_movieClip->tracking;
  MovieTrackingTrack *track;
  VoronoiSite *sites, *site;
  ImBuf *ibuf;
  ListBase *tracksbase;
  ListBase edges = {NULL, NULL};
  int sites_total;
  int i;
  int width = this->getWidth();
  int height = this->getHeight();
  int clip_frame = BKE_movieclip_remap_scene_to_clip_frame(this->m_movieClip, this->m_framenumber);

  if (this->m_trackingObject[0]) {
    MovieTrackingObject *object = BKE_tracking_object_get_named(tracking, this->m_trackingObject);

    if (!object) {
      return NULL;
    }

    tracksbase = BKE_tracking_object_get_tracks(tracking, object);
  }
  else {
    tracksbase = BKE_tracking_get_active_tracks(tracking);
  }

  /* count sites */
  for (track = (MovieTrackingTrack *)tracksbase->first, sites_total = 0; track;
       track = track->next) {
    MovieTrackingMarker *marker = BKE_tracking_marker_get(track, clip_frame);
    float pos[2];

    if (marker->flag & MARKER_DISABLED) {
      continue;
    }

    add_v2_v2v2(pos, marker->pos, track->offset);

    if (!IN_RANGE_INCL(pos[0], 0.0f, 1.0f) || !IN_RANGE_INCL(pos[1], 0.0f, 1.0f)) {
      continue;
    }

    sites_total++;
  }

  if (!sites_total) {
    return NULL;
  }

  BKE_movieclip_user_set_frame(&user, clip_frame);
  ibuf = BKE_movieclip_get_ibuf(this->m_movieClip, &user);

  if (!ibuf) {
    return NULL;
  }

  triangulation = (TriangulationData *)MEM_callocN(sizeof(TriangulationData),
                                                   "keying screen triangulation data");

  sites = (VoronoiSite *)MEM_callocN(sizeof(VoronoiSite) * sites_total,
                                     "keyingscreen voronoi sites");
  track = (MovieTrackingTrack *)tracksbase->first;
  for (track = (MovieTrackingTrack *)tracksbase->first, site = sites; track; track = track->next) {
    MovieTrackingMarker *marker = BKE_tracking_marker_get(track, clip_frame);
    ImBuf *pattern_ibuf;
    int j;
    float pos[2];

    if (marker->flag & MARKER_DISABLED) {
      continue;
    }

    add_v2_v2v2(pos, marker->pos, track->offset);

    if (!IN_RANGE_INCL(pos[0], 0.0f, 1.0f) || !IN_RANGE_INCL(pos[1], 0.0f, 1.0f)) {
      continue;
    }

    pattern_ibuf = BKE_tracking_get_pattern_imbuf(ibuf, track, marker, true, false);

    zero_v3(site->color);

    if (pattern_ibuf) {
      for (j = 0; j < pattern_ibuf->x * pattern_ibuf->y; j++) {
        if (pattern_ibuf->rect_float) {
          add_v3_v3(site->color, &pattern_ibuf->rect_float[4 * j]);
        }
        else {
          unsigned char *rrgb = (unsigned char *)pattern_ibuf->rect;

          site->color[0] += srgb_to_linearrgb((float)rrgb[4 * j + 0] / 255.0f);
          site->color[1] += srgb_to_linearrgb((float)rrgb[4 * j + 1] / 255.0f);
          site->color[2] += srgb_to_linearrgb((float)rrgb[4 * j + 2] / 255.0f);
        }
      }

      mul_v3_fl(site->color, 1.0f / (pattern_ibuf->x * pattern_ibuf->y));
      IMB_freeImBuf(pattern_ibuf);
    }

    site->co[0] = pos[0] * width;
    site->co[1] = pos[1] * height;

    site++;
  }

  IMB_freeImBuf(ibuf);

  BLI_voronoi_compute(sites, sites_total, width, height, &edges);

  BLI_voronoi_triangulate(sites,
                          sites_total,
                          &edges,
                          width,
                          height,
                          &triangulation->triangulated_points,
                          &triangulation->triangulated_points_total,
                          &triangulation->triangles,
                          &triangulation->triangles_total);

  MEM_freeN(sites);
  BLI_freelistN(&edges);

  if (triangulation->triangles_total) {
    rcti *rect;
    rect = triangulation->triangles_AABB = (rcti *)MEM_callocN(
        sizeof(rcti) * triangulation->triangles_total, "voronoi triangulation AABB");

    for (i = 0; i < triangulation->triangles_total; i++, rect++) {
      int *triangle = triangulation->triangles[i];
      VoronoiTriangulationPoint *a = &triangulation->triangulated_points[triangle[0]],
                                *b = &triangulation->triangulated_points[triangle[1]],
                                *c = &triangulation->triangulated_points[triangle[2]];

      float min[2], max[2];

      INIT_MINMAX2(min, max);

      minmax_v2v2_v2(min, max, a->co);
      minmax_v2v2_v2(min, max, b->co);
      minmax_v2v2_v2(min, max, c->co);

      rect->xmin = (int)min[0];
      rect->ymin = (int)min[1];

      rect->xmax = (int)max[0] + 1;
      rect->ymax = (int)max[1] + 1;
    }
  }

  return triangulation;
}

KeyingScreenOperation::RectTriangles *KeyingScreenOperation::createTriangles(PixelsRect *dst_rect)
{
  if (this->m_movieClip == NULL) {
    return NULL;
  }

  mutex.lock();
  if (!this->m_triangulation) {
    this->m_triangulation = buildVoronoiTriangulation();
  }
  mutex.unlock();

  if (!m_triangulation) {
    return NULL;
  }

  RectTriangles *rect_triangles = (RectTriangles *)MEM_callocN(sizeof(RectTriangles),
                                                               "keying screen tile data");

  int triangles_allocated = 0;
  int chunk_size = 20;
  for (int i = 0; i < m_triangulation->triangles_total; i++) {
    if (BLI_rcti_isect(dst_rect, &m_triangulation->triangles_AABB[i], NULL)) {
      rect_triangles->triangles_total++;

      if (rect_triangles->triangles_total > triangles_allocated) {
        if (!rect_triangles->triangles) {
          rect_triangles->triangles = (int *)MEM_mallocN(sizeof(int) * chunk_size,
                                                         "keying screen tile triangles chunk");
        }
        else {
          rect_triangles->triangles = (int *)MEM_reallocN(
              rect_triangles->triangles, sizeof(int) * (triangles_allocated + chunk_size));
        }

        triangles_allocated += chunk_size;
      }

      rect_triangles->triangles[rect_triangles->triangles_total - 1] = i;
    }
  }

  return rect_triangles;
}

void KeyingScreenOperation::deleteTriangles(RectTriangles *rect_triangles)
{
  if (rect_triangles->triangles) {
    MEM_freeN(rect_triangles->triangles);
  }

  MEM_freeN(rect_triangles);
}

void KeyingScreenOperation::execPixels(ExecutionManager &man)
{
  auto cpuWrite = [&](PixelsRect &dst, const WriteRectContext & /*ctx*/) {
    RectTriangles *rect_triangles = createTriangles(&dst);
    TriangulationData *triangulation = m_triangulation;

    CCL::float4 out;

    WRITE_DECL(dst);

    CPU_LOOP_START(dst);

    out = CCL::make_float4(0.0f, 0.0f, 0.0f, 1.0f);
    if (this->m_movieClip && rect_triangles) {
      for (int i = 0; i < rect_triangles->triangles_total; i++) {
        int triangle_idx = rect_triangles->triangles[i];
        rcti *rect = &triangulation->triangles_AABB[triangle_idx];

        if (IN_RANGE_INCL(dst_coords.x, rect->xmin, rect->xmax) &&
            IN_RANGE_INCL(dst_coords.y, rect->ymin, rect->ymax)) {
          int *triangle = triangulation->triangles[triangle_idx];
          VoronoiTriangulationPoint *a = &triangulation->triangulated_points[triangle[0]],
                                    *b = &triangulation->triangulated_points[triangle[1]],
                                    *c = &triangulation->triangulated_points[triangle[2]];
          float w[3];
          float coords[2] = {(float)dst_coords.x, (float)dst_coords.y};
          if (barycentric_coords_v2(a->co, b->co, c->co, coords, w)) {
            if (barycentric_inside_triangle_v2(w)) {
              out.x = a->color[0] * w[0] + b->color[0] * w[1] + c->color[0] * w[2];
              out.y = a->color[1] * w[0] + b->color[1] * w[1] + c->color[1] * w[2];
              out.z = a->color[2] * w[0] + b->color[2] * w[1] + c->color[2] * w[2];

              break;
            }
          }
        }
      }
    }

    WRITE_IMG4(dst, out);

    CPU_LOOP_END;
  };
  return cpuWriteSeek(man, cpuWrite);
}

ResolutionType KeyingScreenOperation::determineResolution(int resolution[2],
                                                          int /*preferredResolution*/[2],

                                                          bool /*setResolution*/)
{
  resolution[0] = 0;
  resolution[1] = 0;

  if (this->m_movieClip) {
    MovieClipUser user = {0};
    int width, height;
    int clip_frame = BKE_movieclip_remap_scene_to_clip_frame(this->m_movieClip,
                                                             this->m_framenumber);

    BKE_movieclip_user_set_frame(&user, clip_frame);
    BKE_movieclip_get_size(this->m_movieClip, &user, &width, &height);

    resolution[0] = width;
    resolution[1] = height;
  }
  return ResolutionType::Fixed;
}
