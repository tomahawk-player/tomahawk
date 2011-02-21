/*
 *  Copyright (C) 2003  Haavard Kvaalen <havardk@xmms.org>
 *
 *  Licensed under GNU LGPL version 2.
 */

#if BYTE_ORDER == BIG_ENDIAN
#define WORDS_BIGENDIAN 1
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    FMT_U8, FMT_S8, FMT_U16_LE, FMT_U16_BE, FMT_U16_NE, FMT_S16_LE, FMT_S16_BE, FMT_S16_NE
}
AFormat;

struct xmms_convert_buffers;

struct xmms_convert_buffers* xmms_convert_buffers_new(void);
/*
 * Free the data assosiated with the buffers, without destroying the
 * context.  The context can be reused.
 */
void xmms_convert_buffers_free(struct xmms_convert_buffers* buf);
void xmms_convert_buffers_destroy(struct xmms_convert_buffers* buf);


typedef int (*convert_func_t)(struct xmms_convert_buffers* buf, void **data, int length);
typedef int (*convert_channel_func_t)(struct xmms_convert_buffers* buf, void **data, int length);
typedef int (*convert_freq_func_t)(struct xmms_convert_buffers* buf, void **data, int length, int ifreq, int ofreq);


convert_func_t xmms_convert_get_func(AFormat output, AFormat input);
convert_channel_func_t xmms_convert_get_channel_func(AFormat fmt, int output, int input);
convert_freq_func_t xmms_convert_get_frequency_func(AFormat fmt, int channels);

#ifdef __cplusplus
}
#endif
