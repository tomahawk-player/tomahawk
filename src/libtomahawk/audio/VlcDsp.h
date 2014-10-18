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

#ifndef VLCDSP_H
#define VLCDSP_H

/*
 * Very tricky technique to enable DSP plugin support
 * There is no other way to do this with libvlc for the moment
 * 
 * A VLC audio filter plugin specificaly made for Tomahawk
 * is necessary to get it functionning
 * 
 * TODO : Check thoroughly if libvlc_media_player_t
 * structure is identical to the one in running libvlccore.so
 * (checking if libVLC version is >= than the one used to compile Tomahawk
 *  and verifying pointers integrity should be enough.
 *  + check if audio filter "dsp" is present in libVLC plugins list)
 * 
 * The define 'VLC_DSP_PLUGIN_ENABLED' better should be a CMake option
*/

// Maybe later :)
//#define VLC_DSP_PLUGIN_ENABLED

#ifdef VLC_DSP_PLUGIN_ENABLED

struct libvlc_media_player_t;

void VlcDspSetup( libvlc_media_player_t* vlcPlayer );

#endif // VLC_DSP_PLUGIN_ENABLED

#endif // VLCDSP_H