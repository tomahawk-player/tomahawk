/*
 *  Copyright (C) 2001-2003  Haavard Kvaalen <havardk@xmms.org>
 *
 *  Licensed under GNU LGPL version 2.
 */

#include <stdlib.h>
#include <stdint.h>
#include "xconvert.h"

// These are adapted from defines in gtypes.h and glibconfig.h
#ifndef FALSE
#define FALSE    ( 0 )
#endif

#ifndef TRUE
#define TRUE    ( !FALSE )
#endif

# define GUINT16_SWAP_LE_BE( val )                          \
    ( ( uint16_t )                                          \
        (                                                   \
            ( uint16_t ) ( ( uint16_t ) ( val ) >> 8 ) |    \
            ( uint16_t ) ( ( uint16_t ) ( val ) << 8 )      \
        )                                                   \
    )

# define GINT16_SWAP_LE_BE( val )    ( ( int16_t ) GUINT16_SWAP_LE_BE ( val ) )

#ifdef WORDS_BIGENDIAN

# define IS_BIG_ENDIAN TRUE

# define GINT16_TO_BE( val )       ( ( int16_t ) ( val ) )
# define GINT16_FROM_BE( val )     ( ( int16_t ) ( val ) )
# define GUINT16_TO_BE( val )      ( ( uint16_t ) ( val ) )
# define GUINT16_FROM_BE( val )    ( ( uint16_t ) ( val ) )

# define GUINT16_TO_LE( val )      ( GUINT16_SWAP_LE_BE ( val ) )
# define GUINT16_FROM_LE( val )    ( GUINT16_SWAP_LE_BE ( val ) )
# define GINT16_TO_LE( val )       ( ( int16_t ) GUINT16_SWAP_LE_BE ( val ) )
# define GINT16_FROM_LE( val )     ( ( int16_t ) GUINT16_SWAP_LE_BE ( val ) )

#else

# define IS_BIG_ENDIAN FALSE

# define GINT16_TO_LE( val )       ( ( int16_t ) ( val ) )
# define GINT16_FROM_LE( val )     ( ( int16_t ) ( val ) )
# define GUINT16_TO_LE( val )      ( ( uint16_t ) ( val ) )
# define GUINT16_FROM_LE( val )    ( ( uint16_t ) ( val ) )

# define GUINT16_TO_BE( val )      ( GUINT16_SWAP_LE_BE ( val ) )
# define GUINT16_FROM_BE( val )    ( GUINT16_SWAP_LE_BE ( val ) )
# define GINT16_TO_BE( val )       ( ( int16_t ) GUINT16_SWAP_LE_BE ( val ) )
# define GINT16_FROM_BE( val )     ( ( int16_t ) GUINT16_SWAP_LE_BE ( val ) )

#endif


struct buffer {
    void *buffer;
    uint size;
};

struct xmms_convert_buffers {
    struct buffer format_buffer, stereo_buffer, freq_buffer;
};

struct xmms_convert_buffers* xmms_convert_buffers_new( void )
{
    return calloc( 1, sizeof( struct xmms_convert_buffers ) );
}

static void* convert_get_buffer( struct buffer *buffer, size_t size )
{
    if ( size > 0 && size <= buffer->size )
        return buffer->buffer;

    buffer->size = size;
    buffer->buffer = realloc( buffer->buffer, size );
    return buffer->buffer;
}

void xmms_convert_buffers_free( struct xmms_convert_buffers* buf )
{
    convert_get_buffer( &buf->format_buffer, 0 );
    convert_get_buffer( &buf->stereo_buffer, 0 );
    convert_get_buffer( &buf->freq_buffer, 0 );
}

void xmms_convert_buffers_destroy( struct xmms_convert_buffers* buf )
{
    if ( !buf )
        return;
    xmms_convert_buffers_free( buf );
    free( buf );
}

static int convert_swap_endian( struct xmms_convert_buffers* buf, void **data, int length )
{
    uint16_t *ptr = *data;
    int i;
    for ( i = 0; i < length; i += 2, ptr++ )
        *ptr = GUINT16_SWAP_LE_BE( *ptr );

    return i;
}

static int convert_swap_sign_and_endian_to_native( struct xmms_convert_buffers* buf, void **data, int length )
{
    uint16_t *ptr = *data;
    int i;
    for ( i = 0; i < length; i += 2, ptr++ )
        *ptr = GUINT16_SWAP_LE_BE( *ptr ) ^ 1 << 15;

    return i;
}

static int convert_swap_sign_and_endian_to_alien( struct xmms_convert_buffers* buf, void **data, int length )
{
    uint16_t *ptr = *data;
    int i;
    for ( i = 0; i < length; i += 2, ptr++ )
        *ptr = GUINT16_SWAP_LE_BE( *ptr ^ 1 << 15 );

    return i;
}

static int convert_swap_sign16( struct xmms_convert_buffers* buf, void **data, int length )
{
    int16_t *ptr = *data;
    int i;
    for ( i = 0; i < length; i += 2, ptr++ )
        *ptr ^= 1 << 15;

    return i;
}

static int convert_swap_sign8( struct xmms_convert_buffers* buf, void **data, int length )
{
    int8_t *ptr = *data;
    int i;
    for ( i = 0; i < length; i++ )
        *ptr++ ^= 1 << 7;

    return i;
}

static int convert_to_8_native_endian( struct xmms_convert_buffers* buf, void **data, int length )
{
    int8_t *output = *data;
    int16_t *input = *data;
    int i;
    for ( i = 0; i < length / 2; i++ )
        *output++ = *input++ >> 8;

    return i;
}

static int convert_to_8_native_endian_swap_sign( struct xmms_convert_buffers* buf, void **data, int length )
{
    int8_t *output = *data;
    int16_t *input = *data;
    int i;
    for ( i = 0; i < length / 2; i++ )
        *output++ = ( *input++ >> 8 ) ^ ( 1 << 7 );

    return i;
}


static int convert_to_8_alien_endian( struct xmms_convert_buffers* buf, void **data, int length )
{
    int8_t *output = *data;
    int16_t *input = *data;
    int i;
    for ( i = 0; i < length / 2; i++ )
        *output++ = *input++ & 0xff;

    return i;
}

static int convert_to_8_alien_endian_swap_sign( struct xmms_convert_buffers* buf, void **data, int length )
{
    int8_t *output = *data;
    int16_t *input = *data;
    int i;
    for ( i = 0; i < length / 2; i++ )
        *output++ = ( *input++ & 0xff ) ^ ( 1 << 7 );

    return i;
}

static int convert_to_16_native_endian( struct xmms_convert_buffers* buf, void **data, int length )
{
    uint8_t *input = *data;
    uint16_t *output;
    int i;
    *data = convert_get_buffer( &buf->format_buffer, length * 2 );
    output = *data;
    for ( i = 0; i < length; i++ )
        *output++ = *input++ << 8;

    return i * 2;
}

static int convert_to_16_native_endian_swap_sign( struct xmms_convert_buffers* buf, void **data, int length )
{
    uint8_t *input = *data;
    uint16_t *output;
    int i;
    *data = convert_get_buffer( &buf->format_buffer, length * 2 );
    output = *data;
    for ( i = 0; i < length; i++ )
        *output++ = ( *input++ << 8 ) ^ ( 1 << 15 );

    return i * 2;
}


static int convert_to_16_alien_endian( struct xmms_convert_buffers* buf, void **data, int length )
{
    uint8_t *input = *data;
    uint16_t *output;
    int i;
    *data = convert_get_buffer( &buf->format_buffer, length * 2 );
    output = *data;
    for ( i = 0; i < length; i++ )
        *output++ = *input++;

    return i * 2;
}

static int convert_to_16_alien_endian_swap_sign( struct xmms_convert_buffers* buf, void **data, int length )
{
    uint8_t *input = *data;
    uint16_t *output;
    int i;
    *data = convert_get_buffer( &buf->format_buffer, length * 2 );
    output = *data;
    for ( i = 0; i < length; i++ )
        *output++ = *input++ ^ ( 1 << 7 );

    return i * 2;
}

static AFormat unnativize( AFormat fmt )
{
    if ( fmt == FMT_S16_NE )
    {
        if ( IS_BIG_ENDIAN )
            return FMT_S16_BE;
        else
            return FMT_S16_LE;
    }
    if ( fmt == FMT_U16_NE )
    {
        if ( IS_BIG_ENDIAN )
            return FMT_U16_BE;
        else
            return FMT_U16_LE;
    }
    return fmt;
}

convert_func_t xmms_convert_get_func( AFormat output, AFormat input )
{
    output = unnativize( output );
    input = unnativize( input );

    if ( output == input )
        return NULL;

    if ( ( output == FMT_U16_BE && input == FMT_U16_LE ) ||
         ( output == FMT_U16_LE && input == FMT_U16_BE ) ||
         ( output == FMT_S16_BE && input == FMT_S16_LE ) ||
         ( output == FMT_S16_LE && input == FMT_S16_BE ) )
        return convert_swap_endian;

    if ( ( output == FMT_U16_BE && input == FMT_S16_BE ) ||
         ( output == FMT_U16_LE && input == FMT_S16_LE ) ||
         ( output == FMT_S16_BE && input == FMT_U16_BE ) ||
         ( output == FMT_S16_LE && input == FMT_U16_LE ) )
        return convert_swap_sign16;

    if ( ( IS_BIG_ENDIAN &&
         ( ( output == FMT_U16_BE && input == FMT_S16_LE ) ||
           ( output == FMT_S16_BE && input == FMT_U16_LE ) ) ) ||
        ( !IS_BIG_ENDIAN &&
         ( ( output == FMT_U16_LE && input == FMT_S16_BE ) ||
           ( output == FMT_S16_LE && input == FMT_U16_BE ) ) ) )
        return convert_swap_sign_and_endian_to_native;

    if ( ( !IS_BIG_ENDIAN &&
         ( ( output == FMT_U16_BE && input == FMT_S16_LE ) ||
           ( output == FMT_S16_BE && input == FMT_U16_LE ) ) ) ||
        ( IS_BIG_ENDIAN &&
         ( ( output == FMT_U16_LE && input == FMT_S16_BE ) ||
           ( output == FMT_S16_LE && input == FMT_U16_BE ) ) ) )
        return convert_swap_sign_and_endian_to_alien;

    if ( ( IS_BIG_ENDIAN &&
         ( ( output == FMT_U8 && input == FMT_U16_BE ) ||
           ( output == FMT_S8 && input == FMT_S16_BE ) ) ) ||
        ( !IS_BIG_ENDIAN &&
         ( ( output == FMT_U8 && input == FMT_U16_LE ) ||
           ( output == FMT_S8 && input == FMT_S16_LE ) ) ) )
        return convert_to_8_native_endian;

    if ( ( IS_BIG_ENDIAN &&
         ( ( output == FMT_U8 && input == FMT_S16_BE ) ||
           ( output == FMT_S8 && input == FMT_U16_BE ) ) ) ||
        ( !IS_BIG_ENDIAN &&
         ( ( output == FMT_U8 && input == FMT_S16_LE ) ||
           ( output == FMT_S8 && input == FMT_U16_LE ) ) ) )
        return convert_to_8_native_endian_swap_sign;

    if ( ( !IS_BIG_ENDIAN &&
         ( ( output == FMT_U8 && input == FMT_U16_BE ) ||
           ( output == FMT_S8 && input == FMT_S16_BE ) ) ) ||
        ( IS_BIG_ENDIAN &&
         ( ( output == FMT_U8 && input == FMT_U16_LE ) ||
           ( output == FMT_S8 && input == FMT_S16_LE ) ) ) )
        return convert_to_8_alien_endian;

    if ( ( !IS_BIG_ENDIAN &&
         ( ( output == FMT_U8 && input == FMT_S16_BE ) ||
           ( output == FMT_S8 && input == FMT_U16_BE ) ) ) ||
        ( IS_BIG_ENDIAN &&
         ( ( output == FMT_U8 && input == FMT_S16_LE ) ||
           ( output == FMT_S8 && input == FMT_U16_LE ) ) ) )
        return convert_to_8_alien_endian_swap_sign;

    if ( ( output == FMT_U8 && input == FMT_S8 ) ||
         ( output == FMT_S8 && input == FMT_U8 ) )
        return convert_swap_sign8;

    if ( ( IS_BIG_ENDIAN &&
         ( ( output == FMT_U16_BE && input == FMT_U8 ) ||
           ( output == FMT_S16_BE && input == FMT_S8 ) ) ) ||
        ( !IS_BIG_ENDIAN &&
         ( ( output == FMT_U16_LE && input == FMT_U8 ) ||
           ( output == FMT_S16_LE && input == FMT_S8 ) ) ) )
        return convert_to_16_native_endian;

    if ( ( IS_BIG_ENDIAN &&
         ( ( output == FMT_U16_BE && input == FMT_S8 ) ||
           ( output == FMT_S16_BE && input == FMT_U8 ) ) ) ||
        ( !IS_BIG_ENDIAN &&
         ( ( output == FMT_U16_LE && input == FMT_S8 ) ||
           ( output == FMT_S16_LE && input == FMT_U8 ) ) ) )
        return convert_to_16_native_endian_swap_sign;

    if ( ( !IS_BIG_ENDIAN &&
         ( ( output == FMT_U16_BE && input == FMT_U8 ) ||
           ( output == FMT_S16_BE && input == FMT_S8 ) ) ) ||
        ( IS_BIG_ENDIAN &&
         ( ( output == FMT_U16_LE && input == FMT_U8 ) ||
           ( output == FMT_S16_LE && input == FMT_S8 ) ) ) )
        return convert_to_16_alien_endian;

    if ( ( !IS_BIG_ENDIAN &&
         ( ( output == FMT_U16_BE && input == FMT_S8 ) ||
           ( output == FMT_S16_BE && input == FMT_U8 ) ) ) ||
        ( IS_BIG_ENDIAN &&
         ( ( output == FMT_U16_LE && input == FMT_S8 ) ||
           ( output == FMT_S16_LE && input == FMT_U8 ) ) ) )
        return convert_to_16_alien_endian_swap_sign;

    //g_warning( "Translation needed, but not available.\n"
    //      "Input: %d; Output %d.", input, output );
    return NULL;
}

static int convert_mono_to_stereo( struct xmms_convert_buffers* buf, void **data, int length, int b16 )
{
    int i;
    void *outbuf = convert_get_buffer( &buf->stereo_buffer, length * 2 );

    if ( b16 )
    {
        uint16_t *output = outbuf, *input = *data;
        for ( i = 0; i < length / 2; i++ )
        {
            *output++ = *input;
            *output++ = *input;
            input++;
        }
    }
    else
    {
        uint8_t *output = outbuf, *input = *data;
        for ( i = 0; i < length; i++ )
        {
            *output++ = *input;
            *output++ = *input;
            input++;
        }
    }
    *data = outbuf;

    return length * 2;
}

static int convert_mono_to_stereo_8( struct xmms_convert_buffers* buf, void **data, int length )
{
    return convert_mono_to_stereo( buf, data, length, FALSE );
}

static int convert_mono_to_stereo_16( struct xmms_convert_buffers* buf, void **data, int length )
{
    return convert_mono_to_stereo( buf, data, length, TRUE );
}

static int convert_stereo_to_mono_u8( struct xmms_convert_buffers* buf, void **data, int length )
{
    uint8_t *output = *data, *input = *data;
    int i;
    for ( i = 0; i < length / 2; i++ )
    {
        uint16_t tmp;
        tmp = *input++;
        tmp += *input++;
        *output++ = tmp / 2;
    }
    return length / 2;
}
static int convert_stereo_to_mono_s8( struct xmms_convert_buffers* buf, void **data, int length )
{
    int8_t *output = *data, *input = *data;
    int i;
    for ( i = 0; i < length / 2; i++ )
    {
        int16_t tmp;
        tmp = *input++;
        tmp += *input++;
        *output++ = tmp / 2;
    }
    return length / 2;
}
static int convert_stereo_to_mono_u16le( struct xmms_convert_buffers* buf, void **data, int length )
{
    uint16_t *output = *data, *input = *data;
    int i;
    for ( i = 0; i < length / 4; i++ )
    {
        uint32_t tmp;
        uint16_t stmp;
        tmp = GUINT16_FROM_LE( *input );
        input++;
        tmp += GUINT16_FROM_LE( *input );
        input++;
        stmp = tmp / 2;
        *output++ = GUINT16_TO_LE( stmp );
    }
    return length / 2;
}

static int convert_stereo_to_mono_u16be( struct xmms_convert_buffers* buf, void **data, int length )
{
    uint16_t *output = *data, *input = *data;
    int i;
    for ( i = 0; i < length / 4; i++ )
    {
        uint32_t tmp;
        uint16_t stmp;
        tmp = GUINT16_FROM_BE( *input );
        input++;
        tmp += GUINT16_FROM_BE( *input );
        input++;
        stmp = tmp / 2;
        *output++ = GUINT16_TO_BE( stmp );
    }
    return length / 2;
}

static int convert_stereo_to_mono_s16le( struct xmms_convert_buffers* buf, void **data, int length )
{
    int16_t *output = *data, *input = *data;
    int i;
    for ( i = 0; i < length / 4; i++ )
    {
        int32_t tmp;
        int16_t stmp;
        tmp = GINT16_FROM_LE( *input );
        input++;
        tmp += GINT16_FROM_LE( *input );
        input++;
        stmp = tmp / 2;
        *output++ = GINT16_TO_LE( stmp );
    }
    return length / 2;
}

static int convert_stereo_to_mono_s16be( struct xmms_convert_buffers* buf, void **data, int length )
{
    int16_t *output = *data, *input = *data;
    int i;
    for ( i = 0; i < length / 4; i++ )
    {
        int32_t tmp;
        int16_t stmp;
        tmp = GINT16_FROM_BE( *input );
        input++;
        tmp += GINT16_FROM_BE( *input );
        input++;
        stmp = tmp / 2;
        *output++ = GINT16_TO_BE( stmp );
    }
    return length / 2;
}

convert_channel_func_t xmms_convert_get_channel_func( AFormat fmt, int output, int input )
{
    fmt = unnativize( fmt );

    if ( output == input )
        return NULL;

    if ( input == 1 && output == 2 )
        switch ( fmt )
        {
            case FMT_U8:
            case FMT_S8:
                return convert_mono_to_stereo_8;
            case FMT_U16_LE:
            case FMT_U16_BE:
            case FMT_S16_LE:
            case FMT_S16_BE:
                return convert_mono_to_stereo_16;
            default:
                //g_warning( "Unknown format: %d"
                //      "No conversion available.", fmt );
                return NULL;
        }
    if ( input == 2 && output == 1 )
        switch ( fmt )
        {
            case FMT_U8:
                return convert_stereo_to_mono_u8;
            case FMT_S8:
                return convert_stereo_to_mono_s8;
            case FMT_U16_LE:
                return convert_stereo_to_mono_u16le;
            case FMT_U16_BE:
                return convert_stereo_to_mono_u16be;
            case FMT_S16_LE:
                return convert_stereo_to_mono_s16le;
            case FMT_S16_BE:
                return convert_stereo_to_mono_s16be;
            default:
                //g_warning( "Unknown format: %d.  "
                //      "No conversion available.", fmt );
                return NULL;
        }

    //g_warning( "Input has %d channels, soundcard uses %d channels\n"
    //      "No conversion is available", input, output );
    return NULL;
}


#define RESAMPLE_STEREO( sample_type, bswap )                   \
do {                                                            \
    const int shift = sizeof ( sample_type );                   \
        int i, in_samples, out_samples, x, delta;               \
    sample_type *inptr = *data, *outptr;                        \
    uint nlen = ( ( ( length >> shift ) * ofreq ) / ifreq );    \
    void *nbuf;                                                 \
    if ( nlen == 0 )                                            \
        break;                                                  \
    nlen <<= shift;                                             \
    if ( bswap )                                                \
        convert_swap_endian( NULL, data, length );              \
    nbuf = convert_get_buffer( &buf->freq_buffer, nlen );       \
    outptr = nbuf;                                              \
    in_samples = length >> shift;                               \
        out_samples = nlen >> shift;                            \
    delta = ( in_samples << 12 ) / out_samples;                 \
    for ( x = 0, i = 0; i < out_samples; i++ )                  \
    {                                                           \
        int x1, frac;                                           \
        x1 = ( x >> 12 ) << 12;                                 \
        frac = x - x1;                                          \
        *outptr++ =                                             \
            ( ( inptr[( x1 >> 12 ) << 1] *                      \
              ( ( 1<<12 ) - frac ) +                            \
              inptr[( ( x1 >> 12 ) + 1 ) << 1] *                \
              frac ) >> 12 );                                   \
        *outptr++ =                                             \
            ( ( inptr[( ( x1 >> 12 ) << 1 ) + 1] *              \
              ( ( 1<<12 ) - frac ) +                            \
              inptr[( ( ( x1 >> 12 ) + 1 ) << 1 ) + 1] *        \
              frac ) >> 12 );                                   \
        x += delta;                                             \
    }                                                           \
    if ( bswap )                                                \
        convert_swap_endian( NULL, &nbuf, nlen );               \
    *data = nbuf;                                               \
    return nlen;                                                \
} while ( 0 )


#define RESAMPLE_MONO( sample_type, bswap )                     \
do {                                                            \
    const int shift = sizeof ( sample_type ) - 1;               \
        int i, x, delta, in_samples, out_samples;               \
    sample_type *inptr = *data, *outptr;                        \
    uint nlen = ( ( ( length >> shift ) * ofreq ) / ifreq );    \
    void *nbuf;                                                 \
    if ( nlen == 0 )                                            \
        break;                                                  \
    nlen <<= shift;                                             \
    if ( bswap )                                                \
        convert_swap_endian( NULL, data, length );              \
    nbuf = convert_get_buffer( &buf->freq_buffer, nlen );       \
    outptr = nbuf;                                              \
    in_samples = length >> shift;                               \
        out_samples = nlen >> shift;                            \
    delta = ( ( length >> shift ) << 12 ) / out_samples;        \
    for ( x = 0, i = 0; i < out_samples; i++ )                  \
    {                                                           \
        int x1, frac;                                           \
        x1 = ( x >> 12 ) << 12;                                 \
        frac = x - x1;                                          \
        *outptr++ =                                             \
            ( ( inptr[x1 >> 12] * ( ( 1<<12 ) - frac ) +        \
              inptr[( x1 >> 12 ) + 1] * frac ) >> 12 );         \
        x += delta;                                             \
    }                                                           \
    if ( bswap )                                                \
        convert_swap_endian( NULL, &nbuf, nlen );               \
    *data = nbuf;                                               \
    return nlen;                                                \
} while ( 0 )

static int convert_resample_stereo_s16ne( struct xmms_convert_buffers* buf, void **data, int length, int ifreq, int ofreq )
{
    RESAMPLE_STEREO( int16_t, FALSE );
    return 0;
}

static int convert_resample_stereo_s16ae( struct xmms_convert_buffers* buf, void **data, int length, int ifreq, int ofreq )
{
    RESAMPLE_STEREO( int16_t, TRUE );
    return 0;
}

static int convert_resample_stereo_u16ne( struct xmms_convert_buffers* buf, void **data, int length, int ifreq, int ofreq )
{
    RESAMPLE_STEREO( uint16_t, FALSE );
    return 0;
}

static int convert_resample_stereo_u16ae( struct xmms_convert_buffers* buf, void **data, int length, int ifreq, int ofreq )
{
    RESAMPLE_STEREO( uint16_t, TRUE );
    return 0;
}

static int convert_resample_mono_s16ne( struct xmms_convert_buffers* buf, void **data, int length, int ifreq, int ofreq )
{
    RESAMPLE_MONO( int16_t, FALSE );
    return 0;
}

static int convert_resample_mono_s16ae( struct xmms_convert_buffers* buf, void **data, int length, int ifreq, int ofreq )
{
    RESAMPLE_MONO( int16_t, TRUE );
    return 0;
}

static int convert_resample_mono_u16ne( struct xmms_convert_buffers* buf, void **data, int length, int ifreq, int ofreq )
{
    RESAMPLE_MONO( uint16_t, FALSE );
    return 0;
}

static int convert_resample_mono_u16ae( struct xmms_convert_buffers* buf, void **data, int length, int ifreq, int ofreq )
{
    RESAMPLE_MONO( uint16_t, TRUE );
    return 0;
}

static int convert_resample_stereo_u8( struct xmms_convert_buffers* buf, void **data, int length, int ifreq, int ofreq )
{
    RESAMPLE_STEREO( uint8_t, FALSE );
    return 0;
}

static int convert_resample_mono_u8( struct xmms_convert_buffers* buf, void **data, int length, int ifreq, int ofreq )
{
    RESAMPLE_MONO( uint8_t, FALSE );
    return 0;
}

static int convert_resample_stereo_s8( struct xmms_convert_buffers* buf, void **data, int length, int ifreq, int ofreq )
{
    RESAMPLE_STEREO( int8_t, FALSE );
    return 0;
}

static int convert_resample_mono_s8( struct xmms_convert_buffers* buf, void **data, int length, int ifreq, int ofreq )
{
    RESAMPLE_MONO( int8_t, FALSE );
    return 0;
}


convert_freq_func_t xmms_convert_get_frequency_func( AFormat fmt, int channels )
{
    fmt = unnativize( fmt );
    //g_message( "fmt %d, channels: %d", fmt, channels );

    if ( channels < 1 || channels > 2 )
    {
        //g_warning( "Unsupported number of channels: %d.  "
        //      "Resample function not available", channels );
        return NULL;
    }
    if ( (  IS_BIG_ENDIAN && fmt == FMT_U16_BE ) ||
         ( !IS_BIG_ENDIAN && fmt == FMT_U16_LE ) )
    {
        if ( channels == 1 )
            return convert_resample_mono_u16ne;
        else
            return convert_resample_stereo_u16ne;
    }
    if ( (  IS_BIG_ENDIAN && fmt == FMT_S16_BE ) ||
         ( !IS_BIG_ENDIAN && fmt == FMT_S16_LE ) )
    {
        if ( channels == 1 )
            return convert_resample_mono_s16ne;
        else
            return convert_resample_stereo_s16ne;
    }
    if ( ( !IS_BIG_ENDIAN && fmt == FMT_U16_BE ) ||
         (  IS_BIG_ENDIAN && fmt == FMT_U16_LE ) )
    {
        if ( channels == 1 )
            return convert_resample_mono_u16ae;
        else
            return convert_resample_stereo_u16ae;
    }
    if ( ( !IS_BIG_ENDIAN && fmt == FMT_S16_BE ) ||
         (  IS_BIG_ENDIAN && fmt == FMT_S16_LE ) )
    {
        if ( channels == 1 )
            return convert_resample_mono_s16ae;
        else
            return convert_resample_stereo_s16ae;
    }
    if ( fmt == FMT_U8 )
    {
        if ( channels == 1 )
            return convert_resample_mono_u8;
        else
            return convert_resample_stereo_u8;
    }
    if ( fmt == FMT_S8 )
    {
        if ( channels == 1 )
            return convert_resample_mono_s8;
        else
            return convert_resample_stereo_s8;
    }
    //g_warning( "Resample function not available"
    //      "Format %d.", fmt );
    return NULL;
}
