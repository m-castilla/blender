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

#include "COM_TrackPositionOperation.h"
#include "BKE_movieclip.h"
#include "BKE_node.h"
#include "BKE_tracking.h"
#include "BLI_listbase.h"
#include "BLI_math.h"
#include "COM_PixelsUtil.h"
#include "MEM_guardedalloc.h"

TrackPositionOperation::TrackPositionOperation() : NodeOperation(), m_markerPos(), m_relativePos()
{
  this->addOutputSocket(SocketType::VALUE);
  this->m_movieClip = NULL;
  this->m_framenumber = 0;
  this->m_trackingObjectName[0] = 0;
  this->m_trackName[0] = 0;
  this->m_axis = 0;
  this->m_position = CMP_TRACKPOS_ABSOLUTE;
  this->m_relativeFrame = 0;
  this->m_speed_output = false;
  this->m_pix_result = 0;
}

void TrackPositionOperation::initExecution()
{
  MovieTracking *tracking = NULL;
  MovieClipUser user = {0};
  MovieTrackingObject *object;

  zero_v2(this->m_markerPos);
  zero_v2(this->m_relativePos);

  if (!this->m_movieClip) {
    return;
  }

  tracking = &this->m_movieClip->tracking;

  int movie_width, movie_height;
  BKE_movieclip_user_set_frame(&user, this->m_framenumber);
  BKE_movieclip_get_size(this->m_movieClip, &user, &movie_width, &movie_height);

  object = BKE_tracking_object_get_named(tracking, this->m_trackingObjectName);
  if (object) {
    MovieTrackingTrack *track;

    track = BKE_tracking_track_get_named(tracking, object, this->m_trackName);

    if (track) {
      MovieTrackingMarker *marker;
      int clip_framenr = BKE_movieclip_remap_scene_to_clip_frame(this->m_movieClip,
                                                                 this->m_framenumber);

      marker = BKE_tracking_marker_get(track, clip_framenr);

      copy_v2_v2(this->m_markerPos, marker->pos);

      if (this->m_speed_output) {
        int relative_clip_framenr = BKE_movieclip_remap_scene_to_clip_frame(this->m_movieClip,
                                                                            this->m_relativeFrame);

        marker = BKE_tracking_marker_get_exact(track, relative_clip_framenr);
        if (marker != NULL && (marker->flag & MARKER_DISABLED) == 0) {
          copy_v2_v2(this->m_relativePos, marker->pos);
        }
        else {
          copy_v2_v2(this->m_relativePos, this->m_markerPos);
        }
        if (this->m_relativeFrame < this->m_framenumber) {
          swap_v2_v2(this->m_relativePos, this->m_markerPos);
        }
      }
      else if (this->m_position == CMP_TRACKPOS_RELATIVE_START) {
        int i;

        for (i = 0; i < track->markersnr; i++) {
          marker = &track->markers[i];

          if ((marker->flag & MARKER_DISABLED) == 0) {
            copy_v2_v2(this->m_relativePos, marker->pos);

            break;
          }
        }
      }
      else if (this->m_position == CMP_TRACKPOS_RELATIVE_FRAME) {
        int relative_clip_framenr = BKE_movieclip_remap_scene_to_clip_frame(this->m_movieClip,
                                                                            this->m_relativeFrame);

        marker = BKE_tracking_marker_get(track, relative_clip_framenr);
        copy_v2_v2(this->m_relativePos, marker->pos);
      }

      // calc single element (pixel) result
      m_pix_result = this->m_markerPos[this->m_axis] - this->m_relativePos[this->m_axis];
      if (this->m_axis == 0) {
        m_pix_result *= movie_width;
      }
      else {
        m_pix_result *= movie_height;
      }
    }
  }

  NodeOperation::initExecution();
}
