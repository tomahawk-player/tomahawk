#ifndef VLCDSPHACK_P_H
#define VLCDSPHACK_P_H

#include "VlcDspHack.h"

#ifdef VLC_DSP_PLUGIN_ENABLED

#include <pthread.h>

#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_media_player.h>
#include <vlc/libvlc_events.h>
#include <vlc/libvlc_version.h>

// TODO : Replace all these copy-pasted strutures by vlc/plugins/?.h includes

typedef struct audio_output audio_output_t;
typedef struct input_resource_t input_resource_t;
typedef struct vlc_object_t vlc_object_t;
typedef struct vlc_list_t vlc_list_t;
typedef int64_t mtime_t;
typedef pthread_mutex_t vlc_mutex_t;

typedef union
{
    int64_t         i_int;
    bool            b_bool;
    float           f_float;
    char *          psz_string;
    void *          p_address;
    vlc_object_t *  p_object;
    vlc_list_t *    p_list;
    mtime_t         i_time;
    struct { int32_t x; int32_t y; } coords;

} vlc_value_t;

struct audio_output
{
    const char *psz_object_type;
    char *psz_header;
    int  i_flags;
    bool b_force;
    void *p_libvlc;
    void *  p_parent;

   void *sys;
    int (*start)(audio_output_t *, void *fmt);
    void (*stop)(audio_output_t *);
    int (*time_get)(audio_output_t *, void *delay);
    void (*play)(audio_output_t *, void *);
    void (*pause)( audio_output_t *, bool pause, mtime_t date);
    void (*flush)( audio_output_t *, bool wait);
    int (*volume_set)(audio_output_t *, float volume);
    int (*mute_set)(audio_output_t *, bool mute);
    int (*device_select)(audio_output_t *, const char *id);
    struct {
        void (*volume_report)(audio_output_t *, float);
        void (*mute_report)(audio_output_t *, bool);
        void (*policy_report)(audio_output_t *, bool);
        void (*device_report)(audio_output_t *, const char *);
        void (*hotplug_report)(audio_output_t *, const char *, const char *);
        int (*gain_request)(audio_output_t *, float);
        void (*restart_request)(audio_output_t *, unsigned);
    } event;
};


struct libvlc_media_player_t
{
    const char *psz_object_type;
    char *psz_header;
    int  i_flags;
    bool b_force;
    void *p_libvlc;
    void *p_parent;

    int                i_refcount;
    vlc_mutex_t        object_lock;

    struct
    {
        void             *p_thread;
        input_resource_t *p_resource;
        vlc_mutex_t       lock;
    } input;

    void *p_libvlc_instance;
    void *p_md;
    void *p_event_manager;
    int   state;
};

extern "C" {
    audio_output_t* input_resource_GetAout(input_resource_t*);
    void input_resource_PutAout(input_resource_t*, audio_output_t*);
    int var_Create(vlc_object_t *, const char *, int );
    int var_SetChecked( vlc_object_t *, const char *, int, vlc_value_t );
    static inline void aout_RestartRequest(audio_output_t *aout, unsigned mode){
        aout->event.restart_request(aout, mode);
    }
};

#endif // VLC_DSP_PLUGIN_ENABLED

#endif // VLCDSPHACK_P_H
