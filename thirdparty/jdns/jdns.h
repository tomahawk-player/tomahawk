/*
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

#ifndef JDNS_H
#define JDNS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*jdns_object_dtor_func)(void *);
typedef void *(*jdns_object_cctor_func)(const void *);

#define JDNS_OBJECT \
	jdns_object_dtor_func dtor; \
	jdns_object_cctor_func cctor;

#define JDNS_OBJECT_NEW(name) \
	(name##_t *)jdns_object_new(sizeof(name##_t), \
		(jdns_object_dtor_func)name##_delete, \
		(jdns_object_cctor_func)name##_copy);

typedef struct jdns_object
{
	JDNS_OBJECT
} jdns_object_t;

void *jdns_object_new(int size, void (*dtor)(void *),
	void *(*cctor)(const void *));
void *jdns_object_copy(const void *a);
void jdns_object_delete(void *a);
void jdns_object_free(void *a);

#define JDNS_LIST_DECLARE(name) \
	JDNS_OBJECT \
	int count; \
	name##_t **item;

typedef struct jdns_list
{
	JDNS_OBJECT
	int count;
	void **item;
	int valueList;
	int autoDelete;
} jdns_list_t;

jdns_list_t *jdns_list_new();
jdns_list_t *jdns_list_copy(const jdns_list_t *a);
void jdns_list_delete(jdns_list_t *a);
void jdns_list_clear(jdns_list_t *a);
void jdns_list_insert(jdns_list_t *a, void *item, int pos);
void jdns_list_insert_value(jdns_list_t *a, const void *item, int pos);
void jdns_list_remove(jdns_list_t *a, void *item);
void jdns_list_remove_at(jdns_list_t *a, int pos);

typedef struct jdns_string
{
	JDNS_OBJECT
	unsigned char *data;
	int size;
} jdns_string_t;

jdns_string_t *jdns_string_new();
jdns_string_t *jdns_string_copy(const jdns_string_t *s);
void jdns_string_delete(jdns_string_t *s);
void jdns_string_set(jdns_string_t *s, const unsigned char *str,
	int str_len);
void jdns_string_set_cstr(jdns_string_t *s, const char *str);

 // overlays jdns_list
typedef struct jdns_stringlist
{
	JDNS_OBJECT
	int count;
	jdns_string_t **item;
} jdns_stringlist_t;

jdns_stringlist_t *jdns_stringlist_new();
jdns_stringlist_t *jdns_stringlist_copy(const jdns_stringlist_t *a);
void jdns_stringlist_delete(jdns_stringlist_t *a);
void jdns_stringlist_append(jdns_stringlist_t *a, const jdns_string_t *str);

typedef struct jdns_address
{
	int isIpv6;
	union
	{
		unsigned long int v4;
		unsigned char *v6; // 16 bytes
	} addr;
	char *c_str;
} jdns_address_t;

jdns_address_t *jdns_address_new();
jdns_address_t *jdns_address_copy(const jdns_address_t *a);
void jdns_address_delete(jdns_address_t *a);
void jdns_address_set_ipv4(jdns_address_t *a, unsigned long int ipv4);
void jdns_address_set_ipv6(jdns_address_t *a, const unsigned char *ipv6);
// return 1 if string was ok, else 0.  Note: IPv4 addresses only!
int jdns_address_set_cstr(jdns_address_t *a, const char *str);
// return 1 if the same, else 0
int jdns_address_cmp(const jdns_address_t *a, const jdns_address_t *b);

// convenient predefined addresses/ports
#define JDNS_UNICAST_PORT    53
#define JDNS_MULTICAST_PORT  5353
jdns_address_t *jdns_address_multicast4_new(); // 224.0.0.251
jdns_address_t *jdns_address_multicast6_new(); // FF02::FB

typedef struct jdns_server
{
	unsigned char *name;
	int port; // SRV only
	int priority;
	int weight; // SRV only
} jdns_server_t;

jdns_server_t *jdns_server_new();
jdns_server_t *jdns_server_copy(const jdns_server_t *s);
void jdns_server_delete(jdns_server_t *s);
void jdns_server_set_name(jdns_server_t *s, const unsigned char *name);

typedef struct jdns_nameserver
{
	jdns_address_t *address;
	int port;
} jdns_nameserver_t;

jdns_nameserver_t *jdns_nameserver_new();
jdns_nameserver_t *jdns_nameserver_copy(const jdns_nameserver_t *a);
void jdns_nameserver_delete(jdns_nameserver_t *a);
void jdns_nameserver_set(jdns_nameserver_t *a, const jdns_address_t *addr,
	int port);

typedef struct jdns_nameserverlist
{
	int count;
	jdns_nameserver_t **item;
} jdns_nameserverlist_t;

jdns_nameserverlist_t *jdns_nameserverlist_new();
jdns_nameserverlist_t *jdns_nameserverlist_copy(
	const jdns_nameserverlist_t *a);
void jdns_nameserverlist_delete(jdns_nameserverlist_t *a);
void jdns_nameserverlist_append(jdns_nameserverlist_t *a,
	const jdns_address_t *addr, int port);

typedef struct jdns_dnshost
{
	jdns_string_t *name;
	jdns_address_t *address;
} jdns_dnshost_t;

typedef struct jdns_dnshostlist
{
	int count;
	jdns_dnshost_t **item;
} jdns_dnshostlist_t;

typedef struct jdns_dnsparams
{
	jdns_nameserverlist_t *nameservers;
	jdns_stringlist_t *domains;
	jdns_dnshostlist_t *hosts;
} jdns_dnsparams_t;

jdns_dnsparams_t *jdns_dnsparams_new();
jdns_dnsparams_t *jdns_dnsparams_copy(jdns_dnsparams_t *a);
void jdns_dnsparams_delete(jdns_dnsparams_t *a);
void jdns_dnsparams_append_nameserver(jdns_dnsparams_t *a,
	const jdns_address_t *addr, int port);
void jdns_dnsparams_append_domain(jdns_dnsparams_t *a,
	const jdns_string_t *domain);
void jdns_dnsparams_append_host(jdns_dnsparams_t *a,
	const jdns_string_t *name, const jdns_address_t *address);

#define JDNS_RTYPE_A         1
#define JDNS_RTYPE_AAAA     28
#define JDNS_RTYPE_MX       15
#define JDNS_RTYPE_SRV      33
#define JDNS_RTYPE_CNAME     5
#define JDNS_RTYPE_PTR      12
#define JDNS_RTYPE_TXT      16
#define JDNS_RTYPE_HINFO    13
#define JDNS_RTYPE_NS        2
#define JDNS_RTYPE_ANY     255

typedef struct jdns_rr
{
	unsigned char *owner;
	int ttl;
	int type;
	int qclass;
	int rdlength;
	unsigned char *rdata;
	int haveKnown;

	union
	{
		jdns_address_t *address;  // for A, AAAA
		jdns_server_t *server;    // for MX, SRV
		unsigned char *name;      // for CNAME, PTR, NS
		jdns_stringlist_t *texts; // for TXT
		struct
		{
			jdns_string_t *cpu;
			jdns_string_t *os;
		} hinfo; // for HINFO
	} data;
} jdns_rr_t;

jdns_rr_t *jdns_rr_new();
jdns_rr_t *jdns_rr_copy(const jdns_rr_t *r);
void jdns_rr_delete(jdns_rr_t *r);
void jdns_rr_set_owner(jdns_rr_t *r, const unsigned char *name);
void jdns_rr_set_record(jdns_rr_t *r, int type, const unsigned char *rdata,
	int rdlength);
void jdns_rr_set_A(jdns_rr_t *r, const jdns_address_t *address);
void jdns_rr_set_AAAA(jdns_rr_t *r, const jdns_address_t *address);
void jdns_rr_set_MX(jdns_rr_t *r, const unsigned char *name, int priority);
void jdns_rr_set_SRV(jdns_rr_t *r, const unsigned char *name, int port,
	int priority, int weight);
void jdns_rr_set_CNAME(jdns_rr_t *r, const unsigned char *name);
void jdns_rr_set_PTR(jdns_rr_t *r, const unsigned char *name);
void jdns_rr_set_TXT(jdns_rr_t *r, const jdns_stringlist_t *texts);
void jdns_rr_set_HINFO(jdns_rr_t *r, const jdns_string_t *cpu,
	const jdns_string_t *os);
void jdns_rr_set_NS(jdns_rr_t *r, const unsigned char *name);
// note: only works on known types
int jdns_rr_verify(const jdns_rr_t *r);

typedef struct jdns_response
{
	int answerCount;
	jdns_rr_t **answerRecords;
	int authorityCount;
	jdns_rr_t **authorityRecords;
	int additionalCount;
	jdns_rr_t **additionalRecords;
} jdns_response_t;

jdns_response_t *jdns_response_new();
jdns_response_t *jdns_response_copy(const jdns_response_t *r);
void jdns_response_delete(jdns_response_t *r);
void jdns_response_append_answer(jdns_response_t *r, const jdns_rr_t *rr);
void jdns_response_append_authority(jdns_response_t *r, const jdns_rr_t *rr);
void jdns_response_append_additional(jdns_response_t *r,
	const jdns_rr_t *rr);

#define JDNS_PUBLISH_SHARED   0x0001
#define JDNS_PUBLISH_UNIQUE   0x0002

#define JDNS_STEP_TIMER       0x0001
#define JDNS_STEP_HANDLE      0x0002

#define JDNS_EVENT_RESPONSE   0x0001
#define JDNS_EVENT_PUBLISH    0x0002
#define JDNS_EVENT_SHUTDOWN   0x0003

#define JDNS_STATUS_SUCCESS   0x0001
#define JDNS_STATUS_NXDOMAIN  0x0002
#define JDNS_STATUS_ERROR     0x0003
#define JDNS_STATUS_TIMEOUT   0x0004
#define JDNS_STATUS_CONFLICT  0x0005

typedef struct jdns_session jdns_session_t;

typedef struct jdns_callbacks
{
	void *app; // user-supplied context

	// time_now:
	//   s: session
	//   app: user-supplied context
	//   return: milliseconds since session started
	int (*time_now)(jdns_session_t *s, void *app);

	// rand_int:
	//   s: session
	//   app: user-supplied context
	//   return: random integer between 0-65535
	int (*rand_int)(jdns_session_t *s, void *app);

	// debug_line:
	//   s: session
	//   app: user-supplied context
	//   str: a line of debug text
	//   return: nothing
	void (*debug_line)(jdns_session_t *s, void *app, const char *str);

	// udp_bind:
	//   s: session
	//   app: user-supplied context
	//   addr: ip address of interface to bind to.  0 for all
	//   port: port of interface to bind to.  0 for any
	//   maddr: multicast address.  0 if not using multicast
	//   return: handle (>0) of bound socket, or 0 on error
	// note: for multicast, the following must be done:
	//   use SO_REUSEPORT to share with other mdns programs
	//   use IP_ADD_MEMBERSHIP to associate addr and maddr
	//   set IP_MULTICAST_TTL to 255
	int (*udp_bind)(jdns_session_t *s, void *app,
		const jdns_address_t *addr, int port,
		const jdns_address_t *maddr);

	// udp_unbind:
	//   s: session
	//   app: user-supplied context
	//   handle: handle of socket obtained with udp_bind
	//   return: nothing
	void (*udp_unbind)(jdns_session_t *s, void *app, int handle);

	// udp_read:
	//   s: session
	//   app: user-supplied context
	//   handle: handle of socket obtained with udp_bind
	//   addr: store ip address of sender
	//   port: store port of sender
	//   buf: store packet content
	//   bufsize: value contains max size, to be changed to real size
	//   return: 1 if packet read, 0 if none available
	int (*udp_read)(jdns_session_t *s, void *app, int handle,
		jdns_address_t *addr, int *port, unsigned char *buf,
		int *bufsize);

	// udp_write:
	//   s: session
	//   app: user-supplied context
	//   handle: handle of socket obtained with udp_bind
	//   addr: ip address of recipient
	//   port: port of recipient
	//   buf: packet content
	//   bufsize: size of packet
	//   return: 1 if packet taken for writing, 0 if this is a bad time
	int (*udp_write)(jdns_session_t *s, void *app, int handle,
		const jdns_address_t *addr, int port, unsigned char *buf,
		int bufsize);
} jdns_callbacks_t;

typedef struct jdns_event
{
	int type;   // JDNS_EVENT
	int id;     // query id or publish id

	// for query, this can be SUCCESS, NXDOMAIN, ERROR, or TIMEOUT
	// for publish, this can be SUCCESS, ERROR, or CONFLICT
	int status;

	// for query
	jdns_response_t *response;
} jdns_event_t;

void jdns_event_delete(jdns_event_t *e);

// jdns_session_new:
//   callbacks: the struct of callbacks
//   return: newly allocated session
jdns_session_t *jdns_session_new(jdns_callbacks_t *callbacks);

// jdns_session_delete:
//   s: session to free
//   return: nothing
void jdns_session_delete(jdns_session_t *s);

// jdns_init_unicast:
//   s: session
//   addr: ip address of interface to bind to.  NULL for all
//   port: port of interface to bind to.  0 for any
//   return: 1 on success, 0 on failure
int jdns_init_unicast(jdns_session_t *s, const jdns_address_t *addr,
	int port);

// jdns_init_multicast:
//   s: session
//   addr: ip address of interface to bind to.  NULL for all
//   port: port of interface to bind to.  0 for any
//   addr: multicast address to associate with.  cannot be NULL
//   return: 1 on success, 0 on failure
int jdns_init_multicast(jdns_session_t *s, const jdns_address_t *addr,
	int port, const jdns_address_t *maddr);

// jdns_shutdown:
//   s: session
//   return: nothing
void jdns_shutdown(jdns_session_t *s);

// jdns_set_nameservers:
//   s: session
//   nslist: list of nameservers
//   return nothing
void jdns_set_nameservers(jdns_session_t *s,
	const jdns_nameserverlist_t *nslist);

// jdns_probe:
//   s: session
//   return: nothing
void jdns_probe(jdns_session_t *s);

// jdns_query:
//   s: session
//   name: the name to look up
//   rtype: the record type
//   return: id of this operation
int jdns_query(jdns_session_t *s, const unsigned char *name, int rtype);

// jdns_cancel_query:
//   s: session
//   id: the operation id to cancel
//   return: nothing
void jdns_cancel_query(jdns_session_t *s, int id);

// jdns_publish:
//   s: session
//   mode: JDNS_PUBLISH shared or unique
//   rec: the record data
//   return: id of this operation
// note: supported record types: A, AAAA, SRV, CNAME, PTR, TXT, and HINFO.
//   if the published type is not one of these, raw rdata must be set.
int jdns_publish(jdns_session_t *s, int mode, const jdns_rr_t *rec);

// jdns_update_publish:
//   s: session
//   id: the operation id to update
//   rec: the record data
//   return: nothing
// note: update only works on successfully published records, and no event
//   is generated for a successful update.
void jdns_update_publish(jdns_session_t *s, int id, const jdns_rr_t *rec);

// jdns_cancel_publish:
//   s: session
//   id: the operation id to cancel
//   return: nothing
void jdns_cancel_publish(jdns_session_t *s, int id);

// jdns_step:
//   s: session
//   return: JDNS_STEP flags OR'd together
int jdns_step(jdns_session_t *s);

// jdns_next_timer:
//   s: session
//   return: milliseconds until timeout
int jdns_next_timer(jdns_session_t *s);

// jdns_set_handle_readable:
//   s: session
//   handle: handle that is now readable
//   return: nothing
void jdns_set_handle_readable(jdns_session_t *s, int handle);

// jdns_set_handle_writable:
//   s: session
//   handle: handle that is now writable
//   return: nothing
void jdns_set_handle_writable(jdns_session_t *s, int handle);

// jdns_next_event:
//   s: session
//   return: newly allocated event, or zero if none are ready
jdns_event_t *jdns_next_event(jdns_session_t *s);

// jdns_system_dnsparams:
//   return: newly allocated dnsparams from the system
jdns_dnsparams_t *jdns_system_dnsparams();

// jdns_set_hold_ids_enabled
//   s: session
//   enabled: whether to enable id holding.  default is 0 (disabled)
//   return: nothing
// normally, when a unicast query completes or any kind of query or publish
//   operation results in an error, the operation is automatically "canceled".
//   when id holding is enabled, the operation still stops internally, but the
//   id value used by that operation is "held" until the application
//   explicitly calls jdns_cancel_query() or jdns_cancel_publish() to release
//   it.  this allows the application to ensure there is no ambiguity when
//   determining which operation a particular event belongs to.  it is disabled
//   be default so as to not introduce memory leaks in existing applications,
//   however new applications really should use it.
void jdns_set_hold_ids_enabled(jdns_session_t *s, int enabled);

#ifdef __cplusplus
}
#endif

#endif
