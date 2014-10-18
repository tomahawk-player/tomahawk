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

#include "VlcDsp_p.h"

#ifdef VLC_DSP_PLUGIN_ENABLED

void
VlcDspSetup( libvlc_media_player_t* vlcPlayer )
{
    vlc_value_t val;

    if ( vlcPlayer->input.p_resource != 0 ) {
        audio_output_t *aout = input_resource_GetAout( vlcPlayer->input.p_resource );
        if ( aout != 0 ) {
            var_Create( ( vlc_object_t* )aout, "audio-filter", VLC_VAR_STRING );
            val.psz_string = (char*)"dsp";
            var_SetChecked( ( vlc_object_t* )aout, "audio-filter", VLC_VAR_STRING, val );
            aout->event.restart_request( aout, AOUT_RESTART_FILTERS );
            input_resource_PutAout( vlcPlayer->input.p_resource, aout );
        }
    }
}

#endif // VLC_DSP_PLUGIN_ENABLED
