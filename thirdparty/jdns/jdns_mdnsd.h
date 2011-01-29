/*
 * Copyright (C) 2005       Jeremie Miller
 * Copyright (C) 2005,2006  Justin Karneges
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

#ifndef JDNS_MDNSD_H
#define JDNS_MDNSD_H

#include "jdns_p.h"

struct mytimeval
{
	unsigned long int tv_sec;   /* seconds */
	unsigned long int tv_usec;  /* microseconds */
};

typedef struct mdnsd_struct *mdnsd; // main daemon data
typedef struct mdnsdr_struct *mdnsdr; // record entry
// answer data
typedef struct mdnsda_struct
{
    unsigned char *name;
    unsigned short int type;
    unsigned long int ttl;
    unsigned long int real_ttl;
    unsigned short int rdlen;
    unsigned char *rdata;
    unsigned long int ip; // A
    unsigned char *rdname; // NS/CNAME/PTR/SRV
    struct { unsigned short int priority, weight, port; } srv; // SRV
} *mdnsda;

///////////
// Global functions
//
// create a new mdns daemon for the given class of names (usually 1) and maximum frame size
mdnsd mdnsd_new(int class, int frame, int port, int (*time_now)(mdnsd d, void *arg), int (*rand_int)(mdnsd d, void *arg), void *arg);
//
// gracefully shutdown the daemon, use mdnsd_out() to get the last packets
void mdnsd_shutdown(mdnsd d);
//
// flush all cached records (network/interface changed)
void mdnsd_flush(mdnsd d);
//
// free given mdnsd (should have used mdnsd_shutdown() first!)
void mdnsd_free(mdnsd d);
//
///////////

///////////
// I/O functions
//
// incoming message from host (to be cached/processed)
void mdnsd_in(mdnsd d, const jdns_packet_t *m, const jdns_response_t *resp, const jdns_address_t *addr, unsigned short int port);
//
// outgoing messge to be delivered to host, returns >0 if one was returned and m/ip/port set
int mdnsd_out(mdnsd d, jdns_packet_t **m, jdns_address_t **addr, unsigned short int *port);
//
// returns the max wait-time until mdnsd_out() needs to be called again 
struct mytimeval *mdnsd_sleep(mdnsd d);
//
////////////

///////////
// Q/A functions
// 
// register a new query
//   answer(record, arg) is called whenever one is found/changes/expires (immediate or anytime after, mdnsda valid until ->ttl==0)
//   either answer returns -1, or another mdnsd_query with a NULL answer will remove/unregister this query
void mdnsd_query(mdnsd d, char *host, int type, int (*answer)(mdnsda a, void *arg), void *arg);
//
// returns the first (if last == NULL) or next answer after last from the cache
//   mdnsda only valid until an I/O function is called
mdnsda mdnsd_list(mdnsd d, char *host, int type, mdnsda last);
//
///////////

///////////
// Publishing functions
//
// create a new unique record (try mdnsda_list first to make sure it's not used)
//   conflict(arg) called at any point when one is detected and unable to recover
//   after the first data is set_*(), any future changes effectively expire the old one and attempt to create a new unique record
mdnsdr mdnsd_unique(mdnsd d, char *host, int type, long int ttl, void (*pubresult)(int result, char *host, int type, void *arg), void *arg);
// 
// create a new shared record
mdnsdr mdnsd_shared(mdnsd d, char *host, int type, long int ttl);
//
// de-list the given record
void mdnsd_done(mdnsd d, mdnsdr r);
//
// these all set/update the data for the given record, nothing is published until they are called
void mdnsd_set_raw(mdnsd d, mdnsdr r, char *data, int len);
void mdnsd_set_host(mdnsd d, mdnsdr r, char *name);
void mdnsd_set_ip(mdnsd d, mdnsdr r, unsigned long int ip);
void mdnsd_set_srv(mdnsd d, mdnsdr r, int priority, int weight, int port, char *name);
//
///////////


#endif
