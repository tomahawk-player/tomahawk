/*
 * Copyright (C) 2005-2008  Justin Karneges
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef JDNS_P_H
#define JDNS_P_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
# define JDNS_OS_WIN
#else
# define JDNS_OS_UNIX
#endif

#if defined(__FreeBSD__) || defined(__DragonFly__)
# define JDNS_OS_FREEBSD
#elif defined(__NetBSD__)
# define JDNS_OS_NETBSD
#elif defined(sun) || defined(__sun)
# define JDNS_OS_SOLARIS
#elif defined(__APPLE__) && (defined(__GNUC__) || defined(__xlC__) || defined(__xlc__))
# define JDNS_OS_MAC
#endif

#ifdef JDNS_OS_WIN
# include <windows.h>
#endif

#ifdef JDNS_OS_UNIX
# include <unistd.h>
# include <netinet/in.h>
#endif

#include "jdns.h"
#include "jdns_packet.h"

// jdns_util.c
void *jdns_alloc(int size);
void *jdns_realloc(void *p, int size);
void jdns_free(void *p);
char *jdns_strdup(const char *s);
unsigned char *jdns_copy_array(const unsigned char *src, int size);
int jdns_domain_cmp(const unsigned char *a, const unsigned char *b);

int jdns_sprintf_s(char *str, int n, const char *format, ...);
int jdns_vsprintf_s(char *str, int n, const char *format, va_list ap);
FILE *jdns_fopen(const char *path, const char *mode);
jdns_string_t *jdns_getenv(const char *name);
char *jdns_strcpy(char *dst, const char *src);

int jdns_string_indexOf(const jdns_string_t *s, unsigned char c, int pos);
jdns_stringlist_t *jdns_string_split(const jdns_string_t *s, unsigned char sep);

jdns_dnshost_t *jdns_dnshost_new();
jdns_dnshost_t *jdns_dnshost_copy(const jdns_dnshost_t *a);
void jdns_dnshost_delete(jdns_dnshost_t *a);
jdns_dnshostlist_t *jdns_dnshostlist_new();
jdns_dnshostlist_t *jdns_dnshostlist_copy(const jdns_dnshostlist_t *a);
void jdns_dnshostlist_delete(jdns_dnshostlist_t *a);
void jdns_dnshostlist_append(jdns_dnshostlist_t *a, const jdns_dnshost_t *host);

jdns_rr_t *jdns_rr_from_resource(const jdns_packet_resource_t *pr, const jdns_packet_t *ref);
void jdns_response_remove_extra(jdns_response_t *r);
void jdns_response_remove_answer(jdns_response_t *r, int pos);

#define alloc_type(type) (type *)jdns_alloc(sizeof(type))
#define _ustrdup(str) (unsigned char *)jdns_strdup((const char *)str)
#define _ustrlen(str) strlen((const char *)str)
#define _ustrcmp(a, b) strcmp((const char *)a, (const char *)b)

#endif
