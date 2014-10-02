#include "VlcDspHack_p.h"

#ifdef VLC_DSP_PLUGIN_ENABLED

void
VlcDspHackInstall( libvlc_media_player_t* vlcPlayer )
{
    if ( vlcPlayer->input.p_resource != 0 ) {
        audio_output_t *aout = input_resource_GetAout( vlcPlayer->input.p_resource );
        if ( aout != 0 ) {
            var_Create( (vlc_object_t*)aout, "audio-filter", 0x0040 /*VLC_VAR_STRING*/ );

            vlc_value_t val;
            val.psz_string = (char*)"dsp";
            var_SetChecked( (vlc_object_t*)aout, "audio-filter", 0x0040 /*VLC_VAR_STRING*/, val );

            aout->event.restart_request( aout, 1 /*AOUT_RESTART_FILTERS*/ );

            input_resource_PutAout( vlcPlayer->input.p_resource, aout );
        }
    }
}

#endif // VLC_DSP_PLUGIN_ENABLED
