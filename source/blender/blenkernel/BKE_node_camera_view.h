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
 * The Original Code is Copyright (C) 2004 Blender Foundation.
 * All rights reserved.
 */

#pragma once

/** \file
 * \ingroup bke
 */

#include "DNA_object_enums.h"

#ifdef __cplusplus
extern "C" {
#endif

struct Camera;
typedef struct NodeCameraJobContext NodeCameraJobContext;
typedef struct ImBuf *(*NodeCameraDrawView)(
    struct Scene *scene,
    struct ViewLayer *view_layer,
    const char *viewname,
    struct Camera *camera, /* if NULL scene camera will be used */
    int frame_offset,
    eDrawType draw_mode,
    struct NodeCameraJobContext *job_context);

#ifdef __cplusplus
}
#endif
