#ifndef VLCDSPHACK_H
#define VLCDSPHACK_H

/*
 * Very tricky technique to enable DSP plugin support
 * There is no other way to do this with libvlc for the moment
 * 
 * A VLC audio filter plugin specificaly made for Tomahawk
 * is necessary to get it functionning
 * 
 * TODO : Check thoroughly if both libvlc_media_player_t and audio_output
 * structures are identical to those in the running libvlccore.so
 * (checking if libVLC version is >= than the one used to compile Tomahawk
 *  and verifying pointers integrity should be enough.
 *  + check if audio filter "dsp" is present in libVLC plugins list)
*/

//#define VLC_DSP_PLUGIN_ENABLED

#ifdef VLC_DSP_PLUGIN_ENABLED

struct libvlc_media_player_t;

void VlcDspHackInstall( libvlc_media_player_t* vlcPlayer );

#endif // VLC_DSP_PLUGIN_ENABLED

#endif // VLCDSPHACK_H