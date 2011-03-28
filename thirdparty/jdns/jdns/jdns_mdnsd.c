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

#include "jdns_mdnsd.h"

#include <stdlib.h>
#include <string.h>

#define QTYPE_A     JDNS_RTYPE_A
#define QTYPE_AAAA  JDNS_RTYPE_AAAA
#define QTYPE_MX    JDNS_RTYPE_MX
#define QTYPE_SRV   JDNS_RTYPE_SRV
#define QTYPE_CNAME JDNS_RTYPE_CNAME
#define QTYPE_PTR   JDNS_RTYPE_PTR
#define QTYPE_TXT   JDNS_RTYPE_TXT
#define QTYPE_HINFO JDNS_RTYPE_HINFO
#define QTYPE_NS    JDNS_RTYPE_NS
#define QTYPE_ANY   JDNS_RTYPE_ANY

// size of query/publish hashes
#define SPRIME 108
// size of cache hash
#define LPRIME 1009
// brute force garbage cleanup frequency, rarely needed (daily default)
#define GC 86400

// maximum number of items to cache.  if an attacker on the LAN advertises
//   a million services, we don't want to crash our program trying to collect
//   them all.  any dns records received beyond this max are ignored.  this
//   means the attacker would succeed in DoS'ing the multicast dns network,
//   but he wouldn't succeed in running our program out of memory.
#define MAX_CACHE 16384

#define bzero(p, size) memset(p, 0, size)

/* messy, but it's the best/simplest balance I can find at the moment
Some internal data types, and a few hashes: querys, answers, cached, and records (published, unique and shared)
Each type has different semantics for processing, both for timeouts, incoming, and outgoing I/O
They inter-relate too, like records affect the querys they are relevant to
Nice things about MDNS: we only publish once (and then ask asked), and only query once, then just expire records we've got cached
*/

struct query
{
    char *name;
    int type;
    unsigned long int nexttry;
    int tries;
    int (*answer)(mdnsda, void *);
    void *arg;
    struct query *next, *list;
};

struct unicast
{
    int id;
    char ipv6;
    unsigned long int to;
    unsigned char to6[16];
    unsigned short int port;
    mdnsdr r;
    struct unicast *next;
};

struct cached
{
    struct mdnsda_struct rr;
    struct query *q;
    struct cached *next;
};

struct mdnsdr_struct
{
    struct mdnsda_struct rr;
    char unique; // # of checks performed to ensure
    int tries;
    void (*pubresult)(int, char *, int, void *);
    void *arg;
    struct mdnsdr_struct *next, *list;
};

struct mdnsd_struct
{
    char shutdown;
    unsigned long int expireall, checkqlist;
    struct mytimeval now, sleep, pause, probe, publish;
    int class, frame;
    struct cached *cache[LPRIME];
    int cache_count;
    struct mdnsdr_struct *published[SPRIME], *probing, *a_now, *a_pause, *a_publish;
    struct unicast *uanswers;
    struct query *queries[SPRIME], *qlist;
    int (*cb_time_now)(struct mdnsd_struct *dp, void *arg);
    int (*cb_rand_int)(struct mdnsd_struct *dp, void *arg);
    void *cb_arg;
    int port;
};

void mygettimeofday(mdnsd d, struct mytimeval *tv)
{
    //struct timeval t;
    //gettimeofday(&t, 0);
    //tv->tv_sec = t.tv_sec;
    //tv->tv_usec = t.tv_usec;

    int msec = d->cb_time_now(d, d->cb_arg);
    tv->tv_sec = msec / 1000;
    tv->tv_usec = (msec % 1000) * 1000;
}

void query_free(struct query *q)
{
    jdns_free(q->name);
    jdns_free(q);
}

void mdnsda_content_free(struct mdnsda_struct *rr)
{
    if(rr->name)
        jdns_free(rr->name);
    if(rr->rdata)
        jdns_free(rr->rdata);
    if(rr->rdname)
        jdns_free(rr->rdname);
}

int _namehash(const char *s)
{
    const unsigned char *name = (const unsigned char *)s;
    unsigned long h = 0, g;

    while (*name)
    { /* do some fancy bitwanking on the string */
        h = (h << 4) + (unsigned long)(*name++);
        if ((g = (h & 0xF0000000UL))!=0)
            h ^= (g >> 24);
        h &= ~g;
    }

    return (int)h;
}

// case-insensitive hash
int _namehash_nocase(const char *s)
{
    int n, len;
    char *low = jdns_strdup(s);
    len = strlen(low);
    for(n = 0; n < len; ++n)
        low[n] = tolower(low[n]);
    n = _namehash(low);
    jdns_free(low);
    return n;
}

// basic linked list and hash primitives
struct query *_q_next(mdnsd d, struct query *q, char *host, int type)
{
    if(q == 0) q = d->queries[_namehash_nocase(host) % SPRIME];
    else q = q->next;
    for(;q != 0; q = q->next)
        if(q->type == type && jdns_domain_cmp((unsigned char *)q->name, (unsigned char *)host))
            return q;
    return 0;
}
struct cached *_c_next(mdnsd d, struct cached *c, char *host, int type)
{
    if(c == 0) c = d->cache[_namehash_nocase(host) % LPRIME];
    else c = c->next;
    for(;c != 0; c = c->next)
        if((type == c->rr.type || type == 255) && jdns_domain_cmp(c->rr.name, (unsigned char *)host))
            return c;
    return 0;
}
mdnsdr _r_next(mdnsd d, mdnsdr r, char *host, int type)
{
    if(r == 0) r = d->published[_namehash_nocase(host) % SPRIME];
    else r = r->next;
    for(;r != 0; r = r->next)
        if(type == r->rr.type && jdns_domain_cmp(r->rr.name, (unsigned char *)host))
            return r;
    return 0;
}

/*
int _rr_len(mdnsda rr)
{
    int len = 12; // name is always compressed (dup of earlier), plus normal stuff
    if(rr->rdata) len += rr->rdlen;
    if(rr->rdname) len += strlen((char *)rr->rdname); // worst case
    if(rr->ip) len += 4;
    if(rr->type == QTYPE_PTR) len += 6; // srv record stuff
    return len;
}
*/

/*
int _a_match(struct resource *r, mdnsda a)
{ // compares new rdata with known a, painfully
    if(strcmp((char *)r->name,(char *)a->name) || r->type != a->type) return 0;
    if(r->type == QTYPE_SRV && !strcmp((char *)r->known.srv.name,(char *)a->rdname) && a->srv.port == r->known.srv.port && a->srv.weight == r->known.srv.weight && a->srv.priority == r->known.srv.priority) return 1;
    if((r->type == QTYPE_PTR || r->type == QTYPE_NS || r->type == QTYPE_CNAME) && !strcmp((char *)a->rdname,(char *)r->known.ns.name)) return 1;
    if(r->rdlength == a->rdlen && !memcmp(r->rdata,a->rdata,r->rdlength)) return 1;
    return 0;
}
*/

int _a_match(const jdns_rr_t *r, mdnsda a)
{
    if(r->type != a->type || !jdns_domain_cmp(r->owner, a->name))
        return 0;
    if(r->type == JDNS_RTYPE_SRV)
    {
        if(jdns_domain_cmp(r->data.server->name, a->rdname)
            && r->data.server->port == a->srv.port
            && r->data.server->priority == a->srv.priority
            && r->data.server->weight == a->srv.weight
        )
            return 1;
    }
    else if(r->type == JDNS_RTYPE_PTR || r->type == JDNS_RTYPE_NS || r->type == JDNS_RTYPE_CNAME)
    {
        if(jdns_domain_cmp(r->data.name, a->rdname))
            return 1;
    }
    else if(r->rdlength == a->rdlen && !memcmp(r->rdata, a->rdata, r->rdlength))
        return 1;

    return 0;
}

// compare time values easily
int _tvdiff(struct mytimeval old, struct mytimeval new)
{
    int udiff = 0;
    if(old.tv_sec != new.tv_sec) udiff = (new.tv_sec - old.tv_sec) * 1000000;
    return (new.tv_usec - old.tv_usec) + udiff;
}

// make sure not already on the list, then insert
void _r_push(mdnsdr *list, mdnsdr r)
{
    mdnsdr cur;
    for(cur = *list; cur != 0; cur = cur->list)
        if(cur == r) return;
    r->list = *list;
    *list = r;
}

// set this r to probing, set next probe time
void _r_probe(mdnsd d, mdnsdr r)
{
    (void)d;
    (void)r;
}

// force any r out right away, if valid
void _r_publish(mdnsd d, mdnsdr r)
{
    if(r->unique && r->unique < 5) return; // probing already
    r->tries = 0;
    d->publish.tv_sec = d->now.tv_sec; d->publish.tv_usec = d->now.tv_usec;
    _r_push(&d->a_publish,r);
}

// send r out asap
void _r_send(mdnsd d, mdnsdr r)
{
    // removing record
    if(r->rr.ttl == 0)
    {
        if(d->a_publish == r)
            d->a_publish = r->list;
        _r_push(&d->a_now, r);
        return;
    }

    if(r->tries < 4)
    { // being published, make sure that happens soon
        d->publish.tv_sec = d->now.tv_sec; d->publish.tv_usec = d->now.tv_usec;
        return;
    }
    if(r->unique)
    { // known unique ones can be sent asap
        _r_push(&d->a_now,r);
        return;
    }
    // set d->pause.tv_usec to random 20-120 msec
    d->pause.tv_sec = d->now.tv_sec;
    //d->pause.tv_usec = d->now.tv_usec + ((d->now.tv_usec % 100) + 20) * 1000;
    d->pause.tv_usec = d->now.tv_usec;
    d->pause.tv_usec += ((d->cb_rand_int(d, d->cb_arg) % 100) + 20) * 1000;
    _r_push(&d->a_pause,r);
}

// create generic unicast response struct
void _u_push(mdnsd d, mdnsdr r, int id, const jdns_address_t *addr, unsigned short int port)
{
    struct unicast *u;
    u = (struct unicast *)jdns_alloc(sizeof(struct unicast));
    bzero(u,sizeof(struct unicast));
    u->r = r;
    u->id = id;
    if(addr->isIpv6)
    {
        u->ipv6 = 1;
        memcpy(u->to6, addr->addr.v6, 16);
    }
    else
    {
        u->ipv6 = 0;
        u->to = addr->addr.v4;
    }
    u->port = port;
    u->next = d->uanswers;
    d->uanswers = u;
}

void _q_reset(mdnsd d, struct query *q)
{
    struct cached *cur = 0;
    q->nexttry = 0;
    q->tries = 0;
    while((cur = _c_next(d,cur,q->name,q->type)))
        if(q->nexttry == 0 || cur->rr.ttl - 7 < q->nexttry) q->nexttry = cur->rr.ttl - 7;
    if(q->nexttry != 0 && q->nexttry < d->checkqlist) d->checkqlist = q->nexttry;
}

void _q_done(mdnsd d, struct query *q)
{ // no more query, update all it's cached entries, remove from lists
    struct cached *c = 0;
    struct query *cur;
    int i = _namehash_nocase(q->name) % SPRIME;
    while((c = _c_next(d,c,q->name,q->type))) c->q = 0;
    if(d->qlist == q) d->qlist = q->list;
    else {
        for(cur=d->qlist;cur->list != q;cur = cur->list);
        cur->list = q->list;
    }
    if(d->queries[i] == q) d->queries[i] = q->next;
    else {
        for(cur=d->queries[i];cur->next != q;cur = cur->next);
        cur->next = q->next;
    }
    query_free(q);
}

void _r_done(mdnsd d, mdnsdr r)
{ // buh-bye, remove from hash and free
    mdnsdr cur = 0;
    int i = _namehash_nocase((char *)r->rr.name) % SPRIME;
    if(d->a_now == r)
        d->a_now = r->list;
    if(d->a_pause == r)
        d->a_pause = r->list;
    if(d->a_publish == r)
        d->a_publish = r->list;
    if(d->published[i] == r) d->published[i] = r->next;
    else {
        for(cur=d->published[i];cur && cur->next != r;cur = cur->next);
        if(cur) cur->next = r->next;
    }
    mdnsda_content_free(&r->rr);
    jdns_free(r);
}

void _q_answer(mdnsd d, struct cached *c)
{ // call the answer function with this cached entry
    if(c->rr.ttl <= d->now.tv_sec) c->rr.ttl = 0;
    if(c->q->answer(&c->rr,c->q->arg) == -1) _q_done(d, c->q);
}

void _conflict(mdnsd d, mdnsdr r)
{
    r->pubresult(0, (char *)r->rr.name,r->rr.type,r->arg);
    mdnsd_done(d,r);
}

void _published(mdnsd d, mdnsdr r)
{
    (void)d;
    r->pubresult(1, (char *)r->rr.name,r->rr.type,r->arg);
}

void _c_expire(mdnsd d, struct cached **list)
{ // expire any old entries in this list
    struct cached *next, *cur = *list, *last = 0;
    while(cur != 0)
    {
        next = cur->next;
        if(d->now.tv_sec >= cur->rr.ttl)
        {
            if(last) last->next = next;
            if(*list == cur) *list = next; // update list pointer if the first one expired
            --(d->cache_count);
            if(cur->q) _q_answer(d,cur);
            mdnsda_content_free(&cur->rr);
            jdns_free(cur);
        }else{
            last = cur;
        }
        cur = next;
    }
}

// brute force expire any old cached records
void _gc(mdnsd d)
{
    int i;
    for(i=0;i<LPRIME;i++)
        if(d->cache[i]) _c_expire(d,&d->cache[i]);
    d->expireall = d->now.tv_sec + GC;
}

struct cached *_find_exact(mdnsd d, const jdns_rr_t *r)
{
    struct cached *c = 0;
    while(1)
    {
        c = _c_next(d, c, (char *)r->owner, r->type);
        if(!c)
            break;
        if(_a_match(r, &c->rr))
            return c;
    }
    return 0;
}

void _cache(mdnsd d, const jdns_rr_t *r)
{
    struct cached *c;
    int i = _namehash_nocase((char *)r->owner) % LPRIME;
    struct cached *same_value;

    // do we already have it?
    //printf("cache: checking for entry: [%s] [%d]\n", r->owner, r->type);
    same_value = _find_exact(d, r);
    if(same_value)
    {
        //printf("already have entry of same value\n");
    }

    if(r->qclass == 32768 + d->class)
    { // cache flush
        // simulate removal of all records for this question,
        //   except if the value hasn't changed
        c = 0;
        while((c = _c_next(d,c,(char *)r->owner,r->type)))
        {
            if(c != same_value)
                c->rr.ttl = 0;
        }
        _c_expire(d,&d->cache[i]);

        // we may have expired same_value here, so check for it again
        same_value = _find_exact(d, r);
    }

    if(r->ttl == 0)
    { // process deletes
        if(same_value)
            same_value->rr.ttl = 0;
        _c_expire(d,&d->cache[i]);
        return;
    }

    if(same_value)
    {
        //printf("updating ttl only\n");

        // only update ttl (this code directly copied from below)
        same_value->rr.ttl = d->now.tv_sec + (r->ttl / 2) + 8;
        same_value->rr.real_ttl = r->ttl;
        return;
    }

    //printf("cache: inserting entry:    [%s] [%d]\n", r->owner, r->type);
    if(d->cache_count >= MAX_CACHE)
        return;

    c = (struct cached *)jdns_alloc(sizeof(struct cached));
    bzero(c,sizeof(struct cached));
    c->rr.name = (unsigned char *)jdns_strdup((char *)r->owner);
    c->rr.type = r->type;
    c->rr.ttl = d->now.tv_sec + (r->ttl / 2) + 8; // XXX hack for now, BAD SPEC, start retrying just after half-waypoint, then expire
    c->rr.real_ttl = r->ttl;
    c->rr.rdlen = r->rdlength;
    c->rr.rdata = jdns_copy_array(r->rdata, r->rdlength);
    switch(r->type)
    {
    case QTYPE_A:
        c->rr.ip = r->data.address->addr.v4;
        break;
    case QTYPE_NS:
    case QTYPE_CNAME:
    case QTYPE_PTR:
        c->rr.rdname = (unsigned char *)jdns_strdup((const char *)r->data.name);
        break;
    case QTYPE_SRV:
        c->rr.rdname = (unsigned char *)jdns_strdup((const char *)r->data.server->name);
        c->rr.srv.port = r->data.server->port;
        c->rr.srv.weight = r->data.server->weight;
        c->rr.srv.priority = r->data.server->priority;
        break;
    }
    c->next = d->cache[i];
    d->cache[i] = c;
    if((c->q = _q_next(d, 0, (char *)r->owner, r->type)))
        _q_answer(d,c);
    if(c->q && c->q->nexttry == 0)
    {
        //printf("cache insert, but nexttry == 0\n");
        _q_reset(d,c->q);
        if(d->checkqlist == 0)
            d->checkqlist = c->q->nexttry;
        //printf("after reset: q->nexttry=%d d->checkqlist=%d\n", c->q->nexttry, d->checkqlist);
    }
}

/*
void _a_copy(struct message *m, mdnsda a)
{ // copy the data bits only
    if(a->rdata) { message_rdata_raw(m, a->rdata, a->rdlen); return; }
    if(a->ip) message_rdata_long(m, a->ip);
    if(a->type == QTYPE_SRV) message_rdata_srv(m, a->srv.priority, a->srv.weight, a->srv.port, a->rdname);
    else if(a->rdname) message_rdata_name(m, a->rdname);
}
*/

void _a_copyq(jdns_list_t *dest, unsigned char *name, unsigned short type, unsigned short class)
{
    jdns_packet_question_t *q = jdns_packet_question_new();
    q->qname = jdns_string_new();
    jdns_string_set_cstr(q->qname, (char *)name);
    q->qtype = type;
    q->qclass = class;
    jdns_list_insert(dest, q, -1);
    jdns_packet_question_delete(q);
}

void _a_copy(jdns_list_t *dest, unsigned char *name, unsigned short type, unsigned short class, unsigned long int ttl, mdnsda a)
{
    jdns_packet_resource_t *r = jdns_packet_resource_new();
    r->qname = jdns_string_new();
    jdns_string_set_cstr(r->qname, (char *)name);
    r->qtype = type;
    r->qclass = class;
    r->ttl = ttl;
    if(a->rdata)
        jdns_packet_resource_add_bytes(r, a->rdata, a->rdlen);
    else if(a->ip)
    {
        unsigned long int ip;
        ip = htonl(a->ip);
        jdns_packet_resource_add_bytes(r, (unsigned char *)&ip, 4);
    }
    else if(a->type == QTYPE_SRV)
    {
        unsigned short priority, weight, port;
        jdns_string_t *name;
        priority = htons(a->srv.priority);
        weight = htons(a->srv.weight);
        port = htons(a->srv.port);
        name = jdns_string_new();
        jdns_string_set_cstr(name, (const char *)a->rdname);
        jdns_packet_resource_add_bytes(r, (unsigned char *)&priority, 2);
        jdns_packet_resource_add_bytes(r, (unsigned char *)&weight, 2);
        jdns_packet_resource_add_bytes(r, (unsigned char *)&port, 2);
        jdns_packet_resource_add_name(r, name);
        jdns_string_delete(name);
    }
    else if(a->rdname)
    {
        jdns_string_t *name;
        name = jdns_string_new();
        jdns_string_set_cstr(name, (const char *)a->rdname);
        jdns_packet_resource_add_name(r, name);
        jdns_string_delete(name);
    }
    jdns_list_insert(dest, r, -1);
    jdns_packet_resource_delete(r);
}

/*
int _r_out(mdnsd d, struct message *m, mdnsdr *list)
{ // copy a published record into an outgoing message
    mdnsdr r; //, next;
    int ret = 0;
    while((r = *list) != 0 && message_packet_len(m) + _rr_len(&r->rr) < d->frame)
    {
        *list = r->list;
        ret++;
        if(r->unique)
            message_an(m, r->rr.name, r->rr.type, (unsigned short)(d->class + 32768), r->rr.ttl);
        else
            message_an(m, r->rr.name, r->rr.type, (unsigned short)d->class, r->rr.ttl);
        _a_copy(m, &r->rr);
        if(r->rr.ttl == 0) _r_done(d,r);
    }
    return ret;
}
*/

int _r_out(mdnsd d, jdns_packet_t *m, mdnsdr *list)
{ // copy a published record into an outgoing message
    mdnsdr r; //, next;
    unsigned short class;
    int ret = 0;
    while((r = *list) != 0)
    {
        *list = r->list;
        ret++;
        class = r->unique ? d->class | 0x8000 : d->class;
        _a_copy(m->answerRecords, r->rr.name, r->rr.type, class, r->rr.ttl, &r->rr);
        if(r->rr.ttl == 0) _r_done(d,r);
    }
    return ret;
}


mdnsd mdnsd_new(int class, int frame, int port, int (*time_now)(mdnsd d, void *arg), int (*rand_int)(mdnsd d, void *arg), void *arg)
{
    //int i;
    mdnsd d;
    d = (mdnsd)jdns_alloc(sizeof(struct mdnsd_struct));
    bzero(d,sizeof(struct mdnsd_struct));
    d->cb_time_now = time_now;
    d->cb_rand_int = rand_int;
    d->cb_arg = arg;
    mygettimeofday(d, &d->now);
    d->expireall = d->now.tv_sec + GC;
    d->class = class;
    d->frame = frame;
    d->cache_count = 0;
    d->port = port;
    return d;
}

void mdnsd_shutdown(mdnsd d)
{ // shutting down, zero out ttl and push out all records
    int i;
    mdnsdr cur,next;
    d->a_now = 0;
    for(i=0;i<SPRIME;i++)
        for(cur = d->published[i]; cur != 0;)
        {
            next = cur->next;
            cur->rr.ttl = 0;
            cur->list = d->a_now;
            d->a_now = cur;
            cur = next;
        }
    d->shutdown = 1;
}

void mdnsd_flush(mdnsd d)
{
    // set all querys to 0 tries
    // free whole cache
    // set all mdnsdr to probing
    // reset all answer lists

    (void)d;
}

void mdnsd_free(mdnsd d)
{
    int i;

    // loop through all hashes, free everything
    // free answers if any

    for(i = 0; i < LPRIME; ++i)
    {
        while(d->cache[i])
        {
            struct cached *cur = d->cache[i];
            d->cache[i] = cur->next;
            mdnsda_content_free(&cur->rr);
            jdns_free(cur);
        }
    }

    for(i = 0; i < SPRIME; ++i)
    {
        while(d->published[i])
        {
            struct mdnsdr_struct *cur = d->published[i];
            d->published[i] = cur->next;
            mdnsda_content_free(&cur->rr);
            jdns_free(cur);
        }
    }

    while(d->uanswers)
    {
        struct unicast *u = d->uanswers;
        d->uanswers = u->next;
        jdns_free(u);
    }

    for(i = 0; i < SPRIME; ++i)
    {
        while(d->queries[i])
        {
            struct query *cur = d->queries[i];
            d->queries[i] = cur->next;
            query_free(cur);
        }
    }

    jdns_free(d);
}

void mdnsd_in(mdnsd d, const jdns_packet_t *m, const jdns_response_t *resp, const jdns_address_t *addr, unsigned short int port)
{
    int i, j;
    mdnsdr r = 0;

    if(d->shutdown) return;

    mygettimeofday(d, &d->now);

    if(m->opts.qr == 0)
    {
        for(i=0;i<m->questions->count;i++)
        { // process each query
            jdns_packet_question_t *pq = (jdns_packet_question_t *)m->questions->item[i];

            if(pq->qclass != d->class || (r = _r_next(d,0,(char *)pq->qname->data,pq->qtype)) == 0) continue;

            // send the matching unicast reply
            if(port != d->port) _u_push(d,r,m->id,addr,port);

            for(;r != 0; r = _r_next(d,r,(char *)pq->qname->data,pq->qtype))
            { // check all of our potential answers
                if(r->unique && r->unique < 5)
                { // probing state, check for conflicts
                    for(j=0;j<resp->authorityCount;j++)
                    { // check all to-be answers against our own
                        jdns_rr_t *ns = resp->authorityRecords[j];
                        if(pq->qtype != ns->type || !jdns_domain_cmp(pq->qname->data, ns->owner)) continue;
                        if(!_a_match(ns,&r->rr))
                        {
                            _conflict(d,r); // answer isn't ours, conflict!

                            // r is invalid after conflict, start all over
                            r = 0;
                            break;
                        }
                    }
                    continue;
                }
                for(j=0;j<resp->answerCount;j++)
                { // check the known answers for this question
                    jdns_rr_t *an = resp->answerRecords[j];
                    if(pq->qtype != an->type || !jdns_domain_cmp(pq->qname->data, an->owner)) continue;
                    if(_a_match(an,&r->rr)) break; // they already have this answer
                }
                if(j == resp->answerCount) _r_send(d,r);
            }
        }
        return;
    }

    for(i=0;i<resp->answerCount;i++)
    { // process each answer, check for a conflict, and cache
        jdns_rr_t *an = resp->answerRecords[i];
        if((r = _r_next(d,0,(char *)an->owner,an->type)) != 0 && r->unique && _a_match(an,&r->rr) == 0) _conflict(d,r);
        _cache(d,an);
    }

    // cache additional records
    for(i=0;i<resp->additionalCount;i++)
    {
        jdns_rr_t *an = resp->additionalRecords[i];
        _cache(d,an);
    }
}

int mdnsd_out(mdnsd d, jdns_packet_t **_m, jdns_address_t **addr, unsigned short int *port)
{
    mdnsdr r;
    int ret = 0;
    jdns_packet_t *m;

    mygettimeofday(d, &d->now);
    //bzero(m,sizeof(struct message));
    m = jdns_packet_new();

    // defaults, multicast
    *port = 0; //htons(5353);
    *addr = 0;
    // *ip = 0; //inet_addr("224.0.0.251");
    m->opts.qr = 1;
    m->opts.aa = 1;

    if(d->uanswers)
    { // send out individual unicast answers
        struct unicast *u = d->uanswers;
        d->uanswers = u->next;
        *port = u->port;
        // *ip = u->to;
        *addr = jdns_address_new();
        if(u->ipv6)
            jdns_address_set_ipv6(*addr, u->to6);
        else
            jdns_address_set_ipv4(*addr, u->to);
        m->id = u->id;
        _a_copyq(m->questions, u->r->rr.name, u->r->rr.type, (unsigned short)d->class);
        _a_copy(m->answerRecords, u->r->rr.name, u->r->rr.type, (unsigned short)d->class, u->r->rr.ttl, &u->r->rr);
        jdns_free(u);
        ret = 1;
        goto end;
    }

//printf("OUT: probing %X now %X pause %X publish %X\n",d->probing,d->a_now,d->a_pause,d->a_publish);

    // accumulate any immediate responses
    if(d->a_now) { ret += _r_out(d, m, &d->a_now); }

    if(d->a_publish && _tvdiff(d->now,d->publish) <= 0)
    { // check to see if it's time to send the publish retries (and unlink if done)
        mdnsdr next, cur = d->a_publish, last = 0;
        unsigned short class;
        while(cur /*&& message_packet_len(m) + _rr_len(&cur->rr) < d->frame*/ )
        {
            next = cur->list;
            ret++; cur->tries++;
            class = cur->unique ? d->class | 0x8000 : d->class;
            _a_copy(m->answerRecords, cur->rr.name, cur->rr.type, class, cur->rr.ttl, &cur->rr);

            if(cur->rr.ttl != 0 && cur->tries < 4)
            {
                last = cur;
                cur = next;
                continue;
            }
            if(d->a_publish == cur) d->a_publish = next;
            if(last) last->list = next;
            if(cur->rr.ttl == 0) _r_done(d,cur);
            cur = next;
        }
        if(d->a_publish)
        {
            d->publish.tv_sec = d->now.tv_sec + 2;
            d->publish.tv_usec = d->now.tv_usec;
        }
    }

    // if we're in shutdown, we're done
    if(d->shutdown)
        goto end;

    // check if a_pause is ready
    if(d->a_pause && _tvdiff(d->now, d->pause) <= 0) ret += _r_out(d, m, &d->a_pause);

    // now process questions
    if(ret)
        goto end;
    m->opts.qr = 0;
    m->opts.aa = 0;

    if(d->probing && _tvdiff(d->now,d->probe) <= 0)
    {
        mdnsdr last = 0;
        for(r = d->probing; r != 0;)
        { // scan probe list to ask questions and process published
            if(r->unique == 4)
            { // done probing, publish
                mdnsdr next = r->list;
                if(d->probing == r)
                    d->probing = r->list;
                else
                    last->list = r->list;
                r->list = 0;
                r->unique = 5;
                _r_publish(d,r);
                _published(d,r);
                r = next;
                continue;
            }
            //message_qd(m, r->rr.name, r->rr.type, (unsigned short)d->class);
            _a_copyq(m->questions, r->rr.name, r->rr.type, (unsigned short)d->class);
            last = r;
            r = r->list;
        }
        for(r = d->probing; r != 0; last = r, r = r->list)
        { // scan probe list again to append our to-be answers
            r->unique++;
            _a_copy(m->authorityRecords, r->rr.name, r->rr.type, (unsigned short)d->class, r->rr.ttl, &r->rr);
            ret++;
        }
        if(ret)
        { // process probes again in the future
            d->probe.tv_sec = d->now.tv_sec;
            d->probe.tv_usec = d->now.tv_usec + 250000;
            goto end;
        }
    }

    if(d->checkqlist && d->now.tv_sec >= d->checkqlist)
    { // process qlist for retries or expirations
        struct query *q;
        struct cached *c;
        unsigned long int nextbest = 0;

        // ask questions first, track nextbest time
        for(q = d->qlist; q != 0; q = q->list)
            if(q->nexttry > 0 && q->nexttry <= d->now.tv_sec && q->tries < 3)
                _a_copyq(m->questions, (unsigned char *)q->name, (unsigned short)q->type, (unsigned short)d->class);
            else if(q->nexttry > 0 && (nextbest == 0 || q->nexttry < nextbest))
                nextbest = q->nexttry;

        // include known answers, update questions
        for(q = d->qlist; q != 0; q = q->list)
        {
            if(q->nexttry == 0 || q->nexttry > d->now.tv_sec) continue;
            if(q->tries == 3)
            { // done retrying, expire and reset
                _c_expire(d,&d->cache[_namehash_nocase(q->name) % LPRIME]);
                _q_reset(d,q);
                continue;
            }
            ret++;
            q->nexttry = d->now.tv_sec + ++q->tries;
            if(nextbest == 0 || q->nexttry < nextbest)
                nextbest = q->nexttry;
            // if room, add all known good entries
            c = 0;
            while((c = _c_next(d,c,q->name,q->type)) != 0 && c->rr.ttl > d->now.tv_sec + 8 /* && message_packet_len(m) + _rr_len(&c->rr) < d->frame */)
            {
                _a_copy(m->answerRecords, (unsigned char *)q->name, (unsigned short)q->type, (unsigned short)d->class, (unsigned long int)(c->rr.ttl - d->now.tv_sec), &c->rr);
            }
        }
        d->checkqlist = nextbest;
    }

    if(d->now.tv_sec > d->expireall)
        _gc(d);

end:
    if(ret)
        *_m = m;
    else
        jdns_packet_delete(m);

    return ret;
}

struct mytimeval *mdnsd_sleep(mdnsd d)
{
    int sec, usec;
    //mdnsdr r;
    d->sleep.tv_sec = d->sleep.tv_usec = 0;
    #define RET while(d->sleep.tv_usec > 1000000) {d->sleep.tv_sec++;d->sleep.tv_usec -= 1000000;} return &d->sleep;

    // first check for any immediate items to handle
    if(d->uanswers || d->a_now) return &d->sleep;

    mygettimeofday(d, &d->now);

    if(d->a_pause)
    { // then check for paused answers
        if((usec = _tvdiff(d->now,d->pause)) > 0) d->sleep.tv_usec = usec;
        RET;
    }

    if(d->probing)
    { // now check for probe retries
        if((usec = _tvdiff(d->now,d->probe)) > 0) d->sleep.tv_usec = usec;
        RET;
    }

    if(d->a_publish)
    { // now check for publish retries
        if((usec = _tvdiff(d->now,d->publish)) > 0) d->sleep.tv_usec = usec;
        RET;
    }

    if(d->checkqlist)
    { // also check for queries with known answer expiration/retry
        if((sec = d->checkqlist - d->now.tv_sec) > 0) d->sleep.tv_sec = sec;
        RET;
    }

    // last resort, next gc expiration
    if((sec = d->expireall - d->now.tv_sec) > 0) d->sleep.tv_sec = sec;
    RET;
}

void mdnsd_query(mdnsd d, char *host, int type, int (*answer)(mdnsda a, void *arg), void *arg)
{
    struct query *q;
    struct cached *cur = 0;
    int i = _namehash_nocase(host) % SPRIME;
    if(!(q = _q_next(d,0,host,type)))
    {
        if(!answer) return;
        q = (struct query *)jdns_alloc(sizeof(struct query));
        bzero(q,sizeof(struct query));
        q->name = jdns_strdup(host);
        q->type = type;
        q->next = d->queries[i];
        q->list = d->qlist;
        d->qlist = d->queries[i] = q;
        q->answer = answer;
        q->arg = arg;
        while((cur = _c_next(d,cur,q->name,q->type)))
        {
            cur->q = q; // any cached entries should be associated
            _q_answer(d,cur); // and reported!
        }
        _q_reset(d,q);
        q->nexttry = d->checkqlist = d->now.tv_sec; // new questin, immediately send out
	return;
    }
    if(!answer)
    { // no answer means we don't care anymore
        _q_done(d,q);
        return;
    }
    q->answer = answer;
    q->arg = arg;
}

mdnsda mdnsd_list(mdnsd d, char *host, int type, mdnsda last)
{
    return (mdnsda)_c_next(d,(struct cached *)last,host,type);
}

mdnsdr mdnsd_shared(mdnsd d, char *host, int type, long int ttl)
{
    int i = _namehash_nocase(host) % SPRIME;
    mdnsdr r;
    r = (mdnsdr)jdns_alloc(sizeof(struct mdnsdr_struct));
    bzero(r,sizeof(struct mdnsdr_struct));
    r->rr.name = (unsigned char *)jdns_strdup(host);
    r->rr.type = type;
    r->rr.ttl = ttl;
    r->next = d->published[i];
    d->published[i] = r;
    return r;
}

mdnsdr mdnsd_unique(mdnsd d, char *host, int type, long int ttl, void (*pubresult)(int result, char *host, int type, void *arg), void *arg)
{
    mdnsdr r;
    r = mdnsd_shared(d,host,type,ttl);
    r->pubresult = pubresult;
    r->arg = arg;
    r->unique = 1;
    _r_push(&d->probing,r);
    d->probe.tv_sec = d->now.tv_sec;
    d->probe.tv_usec = d->now.tv_usec;
    return r;
}

void mdnsd_done(mdnsd d, mdnsdr r)
{
    mdnsdr cur;
    if(r->unique && r->unique < 5)
    { // probing yet, zap from that list first!
        if(d->probing == r) d->probing = r->list;
        else {
            for(cur=d->probing;cur->list != r;cur = cur->list);
            cur->list = r->list;
        }
        _r_done(d,r);
        return;
    }
    r->rr.ttl = 0;
    _r_send(d,r);
}

void mdnsd_set_raw(mdnsd d, mdnsdr r, char *data, int len)
{
    if(r->rr.rdata)
        jdns_free(r->rr.rdata);
    r->rr.rdata = jdns_copy_array((unsigned char*)data, len);
    r->rr.rdlen = len;
    _r_publish(d,r);
}

void mdnsd_set_host(mdnsd d, mdnsdr r, char *name)
{
    jdns_free(r->rr.rdname);
    r->rr.rdname = (unsigned char *)jdns_strdup(name);
    _r_publish(d,r);
}

void mdnsd_set_ip(mdnsd d, mdnsdr r, unsigned long int ip)
{
    r->rr.ip = ip;
    _r_publish(d,r);
}

void mdnsd_set_srv(mdnsd d, mdnsdr r, int priority, int weight, int port, char *name)
{
    r->rr.srv.priority = priority;
    r->rr.srv.weight = weight;
    r->rr.srv.port = port;
    mdnsd_set_host(d,r,name);
}
