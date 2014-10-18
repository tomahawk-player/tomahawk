/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VLCDSP_P_H
#define VLCDSP_P_H

#include "VlcDsp.h"

#ifdef VLC_DSP_PLUGIN_ENABLED

#include <pthread.h>

#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_player.h>
#include <vlc/libvlc_events.h>
#include <vlc/libvlc_version.h>

#define restrict
#define _(s) s

#include <vlc/plugins/vlc_aout.h>
#include <vlc/plugins/vlc_input.h>

typedef struct libvlc_media_player_t
{
    const char *psz_object_type;
    char *psz_header;
    int i_flags;
    bool b_force;
    void *p_libvlc;
    void *p_parent;
    int i_refcount;
    vlc_mutex_t object_lock;
    struct
    {
        void *p_thread;
        input_resource_t *p_resource;
        vlc_mutex_t lock;
    } input;
    void *p_libvlc_instance;
    void *p_md;
    void *p_event_manager;
    int state;
} libvlc_media_player_t;

#endif // VLC_DSP_PLUGIN_ENABLED

#endif // VLCDSP_P_H
