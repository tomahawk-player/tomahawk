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

#include "jdns_p.h"

#include <time.h>

#include "jdns_packet.h"
#include "jdns_mdnsd.h"

#define JDNS_UDP_UNI_OUT_MAX  512
#define JDNS_UDP_UNI_IN_MAX   16384
#define JDNS_UDP_MUL_OUT_MAX  9000
#define JDNS_UDP_MUL_IN_MAX   16384

// cache no more than 7 days
#define JDNS_TTL_MAX          (86400 * 7)
#define JDNS_CACHE_MAX        16384
#define JDNS_CNAME_MAX        16
#define JDNS_QUERY_MAX        4096

//----------------------------------------------------------------------------
// util
//----------------------------------------------------------------------------

// declare this here, but implement it later after we define jdns_session_t
static void _debug_line(jdns_session_t *s, const char *format, ...);

static unsigned char _hex_nibble(unsigned char c)
{
	if(c <= 9)
		return '0' + c;
	else if(c <= 15)
		return 'a' + (c - 10);
	else
		return '?';
}

static void _hex_byte(unsigned char c, unsigned char *dest)
{
	dest[0] = _hex_nibble((unsigned char)(c >> 4));
	dest[1] = _hex_nibble((unsigned char)(c & 0x0f));
}

static jdns_string_t *_make_printable(const unsigned char *str, int size)
{
	unsigned char *buf;
	int n, i;
	jdns_string_t *out;

	if(size == 0)
	{
		out = jdns_string_new();
		jdns_string_set_cstr(out, "");
		return out;
	}

	// make room for the largest possible result
	buf = (unsigned char *)malloc(size * 4);
	i = 0;
	for(n = 0; n < size; ++n)
	{
		unsigned char c = str[n];
		if(c == '\\')
		{
			buf[i++] = '\\';
			buf[i++] = '\\';
		}
		else if(c >= 0x20 && c < 0x7f)
		{
			buf[i++] = c;
		}
		else
		{
			buf[i++] = '\\';
			buf[i++] = 'x';
			_hex_byte(c, buf + i);
			i += 2;
		}
	}

	out = jdns_string_new();
	jdns_string_set(out, buf, i);
	free(buf);
	return out;
}

static jdns_string_t *_make_printable_str(const jdns_string_t *str)
{
	return _make_printable(str->data, str->size);
}

static jdns_string_t *_make_printable_cstr(const char *str)
{
	return _make_printable((const unsigned char *)str, strlen(str));
}

static unsigned char *_fix_input(const unsigned char *in)
{
	unsigned char *out;
	int len;

	// truncate
	len = _ustrlen(in);
	if(len > 254)
		len = 254;

	// add a dot to the end if needed
	if(in[len - 1] != '.' && len < 254)
	{
		out = (unsigned char *)malloc(len + 2); // a dot and a zero
		memcpy(out, in, len);
		out[len] = '.';
		out[len+1] = 0;
		++len;
	}
	else
	{
		out = (unsigned char *)malloc(len + 1); // a zero
		memcpy(out, in, len);
		out[len] = 0;
	}

	return out;
}

static const char *_qtype2str(int qtype)
{
	const char *str;
	switch(qtype)
	{
		case JDNS_RTYPE_A:     str = "A";     break;
		case JDNS_RTYPE_AAAA:  str = "AAAA";  break;
		case JDNS_RTYPE_MX:    str = "MX";    break;
		case JDNS_RTYPE_SRV:   str = "SRV";   break;
		case JDNS_RTYPE_CNAME: str = "CNAME"; break;
		case JDNS_RTYPE_PTR:   str = "PTR";   break;
		case JDNS_RTYPE_TXT:   str = "TXT";   break;
		case JDNS_RTYPE_HINFO: str = "HINFO"; break;
		case JDNS_RTYPE_NS:    str = "NS";    break;
		case JDNS_RTYPE_ANY:   str = "ANY";   break;
		default:               str = "";      break;
	}
	return str;
}

static int _cmp_rdata(const jdns_rr_t *a, const jdns_rr_t *b)
{
	if(a->rdlength != b->rdlength)
		return 0;
	if(memcmp(a->rdata, b->rdata, a->rdlength) != 0)
		return 0;
	return 1;
}

static int _cmp_rr(const jdns_rr_t *a, const jdns_rr_t *b)
{
	if(a->type != b->type)
		return 0;
	if(!jdns_domain_cmp(a->owner, b->owner))
		return 0;
	switch(a->type)
	{
		case JDNS_RTYPE_A:
			if(!jdns_address_cmp(a->data.address, b->data.address))
				return 0;
			break;
		case JDNS_RTYPE_AAAA:
			if(!_cmp_rdata(a, b))
				return 0;
			break;
		case JDNS_RTYPE_MX:
			// unsupported
			return 0;
		case JDNS_RTYPE_SRV:
			if(a->data.server->port != b->data.server->port
				|| a->data.server->priority != b->data.server->priority
				|| a->data.server->weight != b->data.server->weight
				|| !jdns_domain_cmp(a->data.server->name, b->data.server->name)
			)
				return 0;
			break;
		case JDNS_RTYPE_CNAME:
			if(!jdns_domain_cmp(a->data.name, b->data.name))
				return 0;
			break;
		case JDNS_RTYPE_PTR:
			if(!jdns_domain_cmp(a->data.name, b->data.name))
				return 0;
			break;
		case JDNS_RTYPE_TXT:
			if(!_cmp_rdata(a, b))
				return 0;
			break;
		case JDNS_RTYPE_HINFO:
			if(!_cmp_rdata(a, b))
				return 0;
			break;
		case JDNS_RTYPE_NS:
			// unsupported
			return 0;
		default:
			if(!_cmp_rdata(a, b))
				return 0;
			break;
	}
	return 1;
}

static jdns_response_t *_packet2response(const jdns_packet_t *packet, const unsigned char *qname, int qtype, int classmask)
{
	int n;
	jdns_response_t *r;

	r = jdns_response_new();
	for(n = 0; n < packet->answerRecords->count; ++n)
	{
		jdns_packet_resource_t *res = (jdns_packet_resource_t *)packet->answerRecords->item[n];
		jdns_rr_t *rr;
		int put_in_answer;
		if((res->qclass & classmask) != 0x0001)
			continue;
		rr = jdns_rr_from_resource(res, packet);
		if(!rr)
			continue;
		// if qname is set, restrict answers to those that match
		//  the question
		put_in_answer = 1;
		if(qname)
		{
			// name must match. type must either match or be CNAME,
			//   unless the query was for any type
			if((qtype != JDNS_RTYPE_ANY && res->qtype != qtype && res->qtype != JDNS_RTYPE_CNAME) || !jdns_domain_cmp(res->qname->data, qname))
			{
				// put unusable records in additional section
				put_in_answer = 0;
			}
		}
		if(put_in_answer)
			jdns_response_append_answer(r, rr);
		else
			jdns_response_append_additional(r, rr);
		jdns_rr_delete(rr);
	}
	for(n = 0; n < packet->authorityRecords->count; ++n)
	{
		jdns_packet_resource_t *res = (jdns_packet_resource_t *)packet->authorityRecords->item[n];
		jdns_rr_t *rr;
		if((res->qclass & classmask) != 0x0001)
			continue;
		rr = jdns_rr_from_resource(res, packet);
		if(!rr)
			continue;
		jdns_response_append_authority(r, rr);
		jdns_rr_delete(rr);
	}
	for(n = 0; n < packet->additionalRecords->count; ++n)
	{
		jdns_packet_resource_t *res = (jdns_packet_resource_t *)packet->additionalRecords->item[n];
		jdns_rr_t *rr;
		if((res->qclass & classmask) != 0x0001)
			continue;
		rr = jdns_rr_from_resource(res, packet);
		if(!rr)
			continue;
		jdns_response_append_additional(r, rr);
		jdns_rr_delete(rr);
	}
	return r;
}

// size must be 1 to 16
static void _print_hexdump_line(jdns_session_t *s, const unsigned char *buf, int size)
{
	char line[67]; // 3 * 16 + 2 + 16 + zero byte
	int n;

	memset(line, ' ', 66);
	line[66] = 0;
	if(size > 16)
		size = 16;
	for(n = 0; n < size; ++n)
	{
		unsigned char c = buf[n];
		_hex_byte(c, ((unsigned char *)line) + n * 3);
		line[n * 3 + 2] = ' ';
		if(c >= 0x20 && c < 0x7f)
			line[50 + n] = c;
		else
			line[50 + n] = '.';
	}
	_debug_line(s, "  %s", line);
}

static void _print_hexdump(jdns_session_t *s, const unsigned char *buf, int size)
{
	int n;
	int lines;
	int at, len;

	lines = size / 16;
	if(size % 16 != 0)
		++lines;
	for(n = 0; n < lines; ++n)
	{
		at = n * 16;
		if(at + 16 <= size)
			len = 16;
		else
			len = size - at;
		_print_hexdump_line(s, buf + at, len);
	}
}

static void _print_packet_resources(jdns_session_t *s, const jdns_list_t *reslist)
{
	int n;
	for(n = 0; n < reslist->count; ++n)
	{
		jdns_packet_resource_t *r;
		jdns_string_t *str;
		r = (jdns_packet_resource_t *)reslist->item[n];
		str = _make_printable_str(r->qname);
		_debug_line(s, "    %04x/%04x [%s] ttl=%ld size=%d", r->qclass, r->qtype, str->data, r->ttl, r->rdlength);
		jdns_string_delete(str);
	}
}

static void _print_packet(jdns_session_t *s, const jdns_packet_t *packet)
{
	int n;
	_debug_line(s, "Packet:");
	_debug_line(s, "  id:   %d", packet->id);
	_debug_line(s, "  opts: qr:%d, opcode:%d, aa:%d, tc:%d, rd:%d, ra:%d, z:%d, rcode:%d",
		packet->opts.qr, packet->opts.opcode, packet->opts.aa, packet->opts.tc,
		packet->opts.rd, packet->opts.ra, packet->opts.z, packet->opts.rcode);
	_debug_line(s, "  qdcount=%d, ancount=%d, nscount=%d, arcount=%d",
		packet->qdcount, packet->ancount, packet->nscount, packet->arcount);
	if(packet->questions->count > 0)
	{
		_debug_line(s, "  questions: (class/type name)");
		for(n = 0; n < packet->questions->count; ++n)
		{
			jdns_packet_question_t *q;
			jdns_string_t *str;
			q = (jdns_packet_question_t *)packet->questions->item[n];
			str = _make_printable_str(q->qname);
			_debug_line(s, "    %04x/%04x [%s]", q->qclass, q->qtype, str->data);
			jdns_string_delete(str);
		}
	}
	if(packet->answerRecords->count > 0)
	{
		_debug_line(s, "  answerRecords: (class/type owner ttl size)");
		_print_packet_resources(s, packet->answerRecords);
	}
	if(packet->authorityRecords->count > 0)
	{
		_debug_line(s, "  authorityRecords: (class/type owner ttl size)");
		_print_packet_resources(s, packet->authorityRecords);
	}
	if(packet->additionalRecords->count > 0)
	{
		_debug_line(s, "  additionalRecords: (class/type owner ttl size)");
		_print_packet_resources(s, packet->additionalRecords);
	}
}

static void _print_rr(jdns_session_t *s, const jdns_rr_t *rr, const unsigned char *owner)
{
	int n;
	jdns_string_t *ownerstr;

	ownerstr = jdns_string_new();

	// not the expected owner?
	if(!owner || !jdns_domain_cmp(owner, rr->owner))
	{
		unsigned char *buf;
		jdns_string_t *str = _make_printable_cstr((const char *)rr->owner);
		buf = (unsigned char *)malloc(str->size + 3); // " [%s]"
		buf[0] = ' ';
		buf[1] = '[';
		memcpy(buf + 2, str->data, str->size);
		buf[str->size + 2] = ']';
		jdns_string_set(ownerstr, buf, str->size + 3);
		jdns_string_delete(str);
		free(buf);
	}
	else
		jdns_string_set_cstr(ownerstr, "");

	switch(rr->type)
	{
		case JDNS_RTYPE_A:
		{
			_debug_line(s, "    A: [%s] (ttl=%d)%s", rr->data.address->c_str, rr->ttl, ownerstr->data);
			break;
		}
		case JDNS_RTYPE_AAAA:
		{
			_debug_line(s, "    AAAA: [%s] (ttl=%d)%s", rr->data.address->c_str, rr->ttl, ownerstr->data);
			break;
		}
		case JDNS_RTYPE_MX:
		{
			jdns_string_t *str = _make_printable_cstr((const char *)rr->data.server->name);
			_debug_line(s, "    MX: [%s] priority=%d (ttl=%d)%s", str->data, rr->data.server->priority, rr->ttl, ownerstr->data);
			jdns_string_delete(str);
			break;
		}
		case JDNS_RTYPE_SRV:
		{
			jdns_string_t *str = _make_printable_cstr((const char *)rr->data.server->name);
			_debug_line(s, "    SRV: [%s] port=%d priority=%d weight=%d (ttl=%d)%s", str->data, rr->data.server->port, rr->data.server->priority, rr->data.server->weight, rr->ttl, ownerstr->data);
			jdns_string_delete(str);
			break;
		}
		case JDNS_RTYPE_CNAME:
		{
			jdns_string_t *str = _make_printable_cstr((const char *)rr->data.name);
			_debug_line(s, "    CNAME: [%s] (ttl=%d)%s", str->data, rr->ttl, ownerstr->data);
			jdns_string_delete(str);
			break;
		}
		case JDNS_RTYPE_PTR:
		{
			jdns_string_t *str = _make_printable_cstr((const char *)rr->data.name);
			_debug_line(s, "    PTR: [%s] (ttl=%d)%s", str->data, rr->ttl, ownerstr->data);
			jdns_string_delete(str);
			break;
		}
		case JDNS_RTYPE_TXT:
		{
			_debug_line(s, "    TXT: count=%d (ttl=%d)%s", rr->data.texts->count, rr->ttl, ownerstr->data);
			for(n = 0; n < rr->data.texts->count; ++n)
			{
				jdns_string_t *str, *pstr;
				str = rr->data.texts->item[n];
				pstr = _make_printable_str(str);
				_debug_line(s, "      len=%d [%s]", str->size, pstr->data);
				jdns_string_delete(pstr);
			}
			break;
		}
		case JDNS_RTYPE_HINFO:
		{
			jdns_string_t *cpu, *os;
			cpu = _make_printable_str(rr->data.hinfo.cpu);
			os = _make_printable_str(rr->data.hinfo.os);
			_debug_line(s, "    HINFO: [%s] [%s] (ttl=%d)%s", cpu->data, os->data, rr->ttl, ownerstr->data);
			jdns_string_delete(cpu);
			jdns_string_delete(os);
			break;
		}
		case JDNS_RTYPE_NS:
		{
			jdns_string_t *str = _make_printable_cstr((const char *)rr->data.name);
			_debug_line(s, "    NS: [%s] (ttl=%d)%s", str->data, rr->ttl, ownerstr->data);
			jdns_string_delete(str);
			break;
		}
		default:
		{
			_debug_line(s, "    Unknown (%d): %d bytes (ttl=%d)%s", rr->type, rr->rdlength, rr->ttl, ownerstr->data);
			break;
		}
	}
	jdns_string_delete(ownerstr);
}

static void _print_records(jdns_session_t *s, const jdns_response_t *r, const unsigned char *owner)
{
	int n;
	_debug_line(s, "Records:");
	_debug_line(s, "  Answer Records: %d", r->answerCount);
	for(n = 0; n < r->answerCount; ++n)
		_print_rr(s, r->answerRecords[n], owner);
	_debug_line(s, "  Authority Records: %d", r->authorityCount);
	for(n = 0; n < r->authorityCount; ++n)
		_print_rr(s, r->authorityRecords[n], owner);
	_debug_line(s, "  Additional Records: %d", r->additionalCount);
	for(n = 0; n < r->additionalCount; ++n)
		_print_rr(s, r->additionalRecords[n], owner);
}

static int _min(int a, int b)
{
	return (a < b) ? a : b;
}

//----------------------------------------------------------------------------
// jdns_event
//----------------------------------------------------------------------------
jdns_event_t *jdns_event_new()
{
	jdns_event_t *e = alloc_type(jdns_event_t);
	e->response = 0;
	return e;
}

void jdns_event_delete(jdns_event_t *e)
{
	if(!e)
		return;
	jdns_response_delete(e->response);
	jdns_free(e);
}

//----------------------------------------------------------------------------
// jdns - internal types
//----------------------------------------------------------------------------
typedef struct list_item
{
	void (*dtor)(void *);
} list_item_t;

typedef struct list
{
	int count;
	list_item_t **item;
} list_t;

static list_t *list_new()
{
	list_t *l = alloc_type(list_t);
	l->count = 0;
	l->item = 0;
	return l;
}

static void list_delete(list_t *l)
{
	int n;
	if(!l)
		return;
	for(n = 0; n < l->count; ++n)
		l->item[n]->dtor(l->item[n]);
	if(l->item)
		free(l->item);
	jdns_free(l);
}

static void list_insert(list_t *l, void *item, int pos)
{
	list_item_t *i = (list_item_t *)item;
	if(!l->item)
		l->item = (list_item_t **)malloc(sizeof(list_item_t *));
	else
		l->item = (list_item_t **)realloc(l->item, sizeof(list_item_t *) * (l->count + 1));
	if(pos != -1)
		memmove(l->item + pos + 1, l->item + pos, (l->count - pos) * sizeof(list_item_t *));
	else
		pos = l->count;
	l->item[pos] = i;
	++l->count;
}

static void list_remove(list_t *l, void *item)
{
	int n;
	list_item_t *i = (list_item_t *)item;
	int pos = -1;
	for(n = 0; n < l->count; ++n)
	{
		if(l->item[n] == i)
		{
			pos = n;
			break;
		}
	}
	if(pos == -1)
		return;

	i->dtor(i);
	if(l->count > 1)
	{
		memmove(l->item + pos, l->item + pos + 1, (l->count - pos - 1) * sizeof(list_item_t *));
		--l->count;
	}
	else
	{
		free(l->item);
		l->item = 0;
		l->count = 0;
	}
}

typedef struct name_server
{
	void (*dtor)(struct name_server *);
	int id;
	jdns_address_t *address;
	int port;
} name_server_t;

static void name_server_delete(name_server_t *ns);

static name_server_t *name_server_new()
{
	name_server_t *ns = alloc_type(name_server_t);
	ns->dtor = name_server_delete;
	ns->address = 0;
	return ns;
}

void name_server_delete(name_server_t *ns)
{
	if(!ns)
		return;
	jdns_address_delete(ns->address);
	jdns_free(ns);
}

int _intarray_indexOf(int *array, int count, int val)
{
	int n;
	for(n = 0; n < count; ++n)
	{
		if(array[n] == val)
			return n;
	}
	return -1;
}

int _intarray_add(int **array, int *count, int val)
{
	int *p;
	if(!*array)
		p = (int *)malloc(sizeof(int));
	else
		p = (int *)realloc(*array, sizeof(int) * (*count + 1));
	if(!p)
		return 0;
	*array = p;
	(*array)[*count] = val;
	++(*count);
	return 1;
}

void _intarray_remove(int **array, int *count, int pos)
{
	int *p;
	if(*count > 1)
	{
		memmove(*array + pos, *array + pos + 1, (*count - pos - 1) * sizeof(int));
		--(*count);
		p = (int *)realloc(*array, sizeof(int) * (*count));
		if(p)
			*array = p;
	}
	else
	{
		free(*array);
		*array = 0;
		*count = 0;
	}
}

typedef struct query
{
	void (*dtor)(struct query *);

	int id;

	// user request ids
	int req_ids_count;
	int *req_ids;

	// packet id
	int dns_id;

	// what we are looking up
	unsigned char *qname;
	int qtype;

	// how many transmission attempts we have done.  note this
	//  is not actually how many packets have been sent, since
	//  it is possible for the first transmission to send many
	//  at once.  this variable lets us decide when to give up.
	//  (idea taken from qdns).
	// set to -1 to deactivate (stop sending packets)
	int step;

	// which nameservers we've tried (stored as a list of ids)
	int servers_tried_count;
	int *servers_tried;

	// which servers we shouldn't try again
	int servers_failed_count;
	int *servers_failed;

	// flag to indicate whether or not we've tried all available
	//  nameservers already.  this means that all future
	//  transmissions are likely repeats, and should be slowed
	//  down.
	int retrying;

	// flag to indicate if we've received nxdomain as an error so far
	int nxdomain;

	// holds a timeout for the next step (time_start == -1 means no timer)
	int time_start;
	int time_next;

	// whether or not to look in the cache for this query
	int trycache;

	// cname subquerying.  only cname_parent or cname_child may be set,
	//  never both.
	int cname_chain_count;
	struct query *cname_parent;
	struct query *cname_child;

	// accumulates known multicast records to prevent duplicates
	jdns_response_t *mul_known;
} query_t;

void query_delete(query_t *q);

query_t *query_new()
{
	query_t *q = alloc_type(query_t);
	q->dtor = query_delete;
	q->req_ids_count = 0;
	q->req_ids = 0;
	q->qname = 0;
	q->servers_tried_count = 0;
	q->servers_tried = 0;
	q->servers_failed_count = 0;
	q->servers_failed = 0;
	q->nxdomain = 0;
	q->cname_chain_count = 0;
	q->cname_parent = 0;
	q->cname_child = 0;
	q->mul_known = 0;
	return q;
}

void query_delete(query_t *q)
{
	if(!q)
		return;
	if(q->req_ids)
		free(q->req_ids);
	if(q->qname)
		free(q->qname);
	if(q->servers_tried)
		free(q->servers_tried);
	if(q->servers_failed)
		free(q->servers_failed);
	jdns_response_delete(q->mul_known);
	jdns_free(q);
}

int query_have_req_id(const query_t *q, int req_id)
{
	if(_intarray_indexOf(q->req_ids, q->req_ids_count, req_id) != -1)
		return 1;
	return 0;
}

void query_add_req_id(query_t *q, int req_id)
{
	_intarray_add(&q->req_ids, &q->req_ids_count, req_id);
}

void query_remove_req_id(query_t *q, int req_id)
{
	int pos;

	pos = _intarray_indexOf(q->req_ids, q->req_ids_count, req_id);
	if(pos != -1)
		_intarray_remove(&q->req_ids, &q->req_ids_count, pos);
}

int query_server_tried(const query_t *q, int ns_id)
{
	if(_intarray_indexOf(q->servers_tried, q->servers_tried_count, ns_id) != -1)
		return 1;
	return 0;
}

void query_add_server_tried(query_t *q, int ns_id)
{
	_intarray_add(&q->servers_tried, &q->servers_tried_count, ns_id);
}

int query_server_failed(const query_t *q, int ns_id);

void query_clear_servers_tried(query_t *q)
{
	int n;

	// all failed servers must continue to be considered tried servers, so
	//   only clear tried servers that haven't failed
	for(n = 0; n < q->servers_tried_count; ++n)
	{
		if(!query_server_failed(q, q->servers_tried[n]))
		{
			_intarray_remove(&q->servers_tried, &q->servers_tried_count, n);
			--n; // adjust position
		}
	}
}

int query_server_failed(const query_t *q, int ns_id)
{
	if(_intarray_indexOf(q->servers_failed, q->servers_failed_count, ns_id) != -1)
		return 1;
	return 0;
}

void query_add_server_failed(query_t *q, int ns_id)
{
	_intarray_add(&q->servers_failed, &q->servers_failed_count, ns_id);
}

void query_name_server_gone(query_t *q, int ns_id)
{
	int pos;

	pos = _intarray_indexOf(q->servers_tried, q->servers_tried_count, ns_id);
	if(pos != -1)
		_intarray_remove(&q->servers_tried, &q->servers_tried_count, pos);

	pos = _intarray_indexOf(q->servers_failed, q->servers_failed_count, ns_id);
	if(pos != -1)
		_intarray_remove(&q->servers_failed, &q->servers_failed_count, pos);
}

typedef struct datagram
{
	void (*dtor)(struct datagram *);
	int handle;
	jdns_address_t *dest_address;
	int dest_port;
	unsigned char *data;
	int size;

	// query association
	query_t *query;
	int query_send_type; // 0 == normal, 1 == first step send-all

	// name server association
	int ns_id;
} datagram_t;

void datagram_delete(datagram_t *a);

datagram_t *datagram_new()
{
	datagram_t *a = alloc_type(datagram_t);
	a->dtor = datagram_delete;
	a->dest_address = 0;
	a->data = 0;
	a->size = 0;
	a->query = 0;
	return a;
}

void datagram_delete(datagram_t *a)
{
	if(!a)
		return;
	jdns_address_delete(a->dest_address);
	if(a->data)
		free(a->data);
	jdns_free(a);
}

typedef struct cache_item
{
	void (*dtor)(struct cache_item *);
	unsigned char *qname;
	int qtype;
	int time_start;
	int ttl;
	jdns_rr_t *record; // if zero, nxdomain is assumed
} cache_item_t;

void cache_item_delete(cache_item_t *e);

cache_item_t *cache_item_new()
{
	cache_item_t *a = alloc_type(cache_item_t);
	a->dtor = cache_item_delete;
	a->qname = 0;
	a->record = 0;
	return a;
}

void cache_item_delete(cache_item_t *a)
{
	if(!a)
		return;
	if(a->qname)
		free(a->qname);
	jdns_rr_delete(a->record);
	jdns_free(a);
}

typedef struct event
{
	void (*dtor)(struct event *);
	jdns_event_t *event;
} event_t;

void event_delete(event_t *e);

event_t *event_new()
{
	event_t *e = alloc_type(event_t);
	e->dtor = event_delete;
	e->event = 0;
	return e;
}

void event_delete(event_t *e)
{
	if(!e)
		return;
	jdns_event_delete(e->event);
	jdns_free(e);
}

typedef struct published_item
{
	void (*dtor)(struct published_item *);
	int id;
	int mode;
	unsigned char *qname;
	int qtype;
	mdnsdr rec;
	jdns_rr_t *rr;
} published_item_t;

void published_item_delete(published_item_t *a);

published_item_t *published_item_new()
{
	published_item_t *a = alloc_type(published_item_t);
	a->dtor = published_item_delete;
	a->qname = 0;
	a->rec = 0;
	a->rr = 0;
	return a;
}

void published_item_delete(published_item_t *a)
{
	if(!a)
		return;
	if(a->qname)
		free(a->qname);
	jdns_rr_delete(a->rr);
	jdns_free(a);
}

//----------------------------------------------------------------------------
// jdns
//----------------------------------------------------------------------------
struct jdns_session
{
	jdns_callbacks_t cb;
	int mode;
	int shutdown;
	int next_qid;
	int next_req_id;
	int last_time;
	int next_timer;
	int next_name_server_id;
	int handle;
	int handle_readable, handle_writable;
	int port;
	list_t *name_servers;
	list_t *queries;
	list_t *outgoing;
	list_t *events;
	list_t *cache;

	// for blocking req_ids from reuse until user explicitly releases
	int do_hold_req_ids;
	int held_req_ids_count;
	int *held_req_ids;

	// mdns
	mdnsd mdns;
	list_t *published;
	jdns_address_t *maddr;
};

jdns_session_t *jdns_session_new(jdns_callbacks_t *callbacks)
{
	jdns_session_t *s = alloc_type(jdns_session_t);
	memcpy(&s->cb, callbacks, sizeof(jdns_callbacks_t));
	s->shutdown = 0;
	s->next_qid = 0;
	s->next_req_id = 1;
	s->last_time = 0;
	s->next_timer = 0;
	s->next_name_server_id = 0;
	s->handle = 0;
	s->handle_readable = 0;
	s->handle_writable = 1;
	s->port = 0;
	s->name_servers = list_new();
	s->queries = list_new();
	s->outgoing = list_new();
	s->events = list_new();
	s->cache = list_new();

	s->do_hold_req_ids = 0;
	s->held_req_ids_count = 0;
	s->held_req_ids = 0;

	s->mdns = 0;
	s->published = list_new();
	s->maddr = 0;

	return s;
}

void jdns_session_delete(jdns_session_t *s)
{
	if(!s)
		return;
	if(s->handle)
		s->cb.udp_unbind(s, s->cb.app, s->handle);
	list_delete(s->name_servers);
	list_delete(s->queries);
	list_delete(s->outgoing);
	list_delete(s->events);
	list_delete(s->cache);

	if(s->held_req_ids)
		free(s->held_req_ids);

	if(s->mdns)
		mdnsd_free(s->mdns);

	list_delete(s->published);
	jdns_address_delete(s->maddr);

	free(s);
}

// declare some internal functions
static int _callback_time_now(mdnsd d, void *arg);
static int _callback_rand_int(mdnsd d, void *arg);

static void _append_event(jdns_session_t *s, jdns_event_t *event);
static void _append_event_and_hold_id(jdns_session_t *s, jdns_event_t *event);
static void _remove_name_server_datagrams(jdns_session_t *s, int ns_id);
static void _remove_query_datagrams(jdns_session_t *s, const query_t *q);

static int _unicast_query(jdns_session_t *s, const unsigned char *name, int qtype);
static void _unicast_cancel(jdns_session_t *s, query_t *q);
static int _multicast_query(jdns_session_t *s, const unsigned char *name, int qtype);
static void _multicast_cancel(jdns_session_t *s, int req_id);
static int _multicast_publish(jdns_session_t *s, int mode, const jdns_rr_t *rr);
static void _multicast_update_publish(jdns_session_t *s, int id, const jdns_rr_t *rr);
static void _multicast_cancel_publish(jdns_session_t *s, int id);
static void _multicast_flush(jdns_session_t *s);

static int jdns_step_unicast(jdns_session_t *s, int now);
static int jdns_step_multicast(jdns_session_t *s, int now);

static void _hold_req_id(jdns_session_t *s, int req_id)
{
	int pos;

	// make sure we don't hold an id twice
	pos = _intarray_indexOf(s->held_req_ids, s->held_req_ids_count, req_id);
	if(pos != -1)
		return;

	_intarray_add(&s->held_req_ids, &s->held_req_ids_count, req_id);
}

static void _unhold_req_id(jdns_session_t *s, int req_id)
{
	int pos;

	pos = _intarray_indexOf(s->held_req_ids, s->held_req_ids_count, req_id);
	if(pos != -1)
		_intarray_remove(&s->held_req_ids, &s->held_req_ids_count, pos);
}

static void _set_hold_ids_enabled(jdns_session_t *s, int enabled)
{
	if(enabled && !s->do_hold_req_ids)
	{
		s->do_hold_req_ids = 1;
	}
	else if(!enabled && s->do_hold_req_ids)
	{
		s->do_hold_req_ids = 0;

		if(s->held_req_ids)
			free(s->held_req_ids);
		s->held_req_ids = 0;
		s->held_req_ids_count = 0;
	}
}

static int _int_wrap(int *src, int start)
{
	int x;
	x = (*src)++;
	if(*src < start)
		*src = start;
	return x;
}

// starts at 0
static int get_next_qid(jdns_session_t *s)
{
	int n, id;
	id = -1;
	while(id == -1)
	{
		id = _int_wrap(&s->next_qid, 0);
		for(n = 0; n < s->queries->count; ++n)
		{
			if(((query_t *)s->queries->item[n])->id == id)
			{
				id = -1;
				break;
			}
		}
	}
	return id;
}

// starts at 1
static int get_next_req_id(jdns_session_t *s)
{
	int n, k, id;
	id = -1;
	while(id == -1)
	{
		id = _int_wrap(&s->next_req_id, 1);

		// no query using this?
		for(n = 0; n < s->queries->count; ++n)
		{
			query_t *q = (query_t *)s->queries->item[n];
			for(k = 0; k < q->req_ids_count; ++k)
			{
				if(q->req_ids[k] == id)
				{
					id = -1;
					break;
				}
			}
			if(id == -1)
				break;
		}

		// no publish using this?
		for(n = 0; n < s->published->count; ++n)
		{
			if(((published_item_t *)s->published->item[n])->id == id)
			{
				id = -1;
				break;
			}
		}

		// successful unicast queries or any kind of error result in
		//   events for actions that are no longer active.  we need
		//   to make sure ids for these actions are not reassigned
		//   until the user explicitly releases them
		for(n = 0; n < s->held_req_ids_count; ++n)
		{
			if(s->held_req_ids[n] == id)
			{
				id = -1;
				break;
			}
		}
	}
	return id;
}

// random number fitting in 16 bits
static int get_next_dns_id(jdns_session_t *s)
{
	int n, id, active_ids;
	active_ids = 0;
	id = -1;
	while(id == -1)
	{
		// use random number for dns id
		id = s->cb.rand_int(s, s->cb.app) & 0xffff;

		for(n = 0; n < s->queries->count; ++n)
		{
			query_t *q = (query_t *)s->queries->item[n];
			if(q->dns_id != -1)
			{
				++active_ids;
				if(active_ids >= JDNS_QUERY_MAX)
					return -1;

				if(q->dns_id == id)
				{
					id = -1;
					break;
				}
			}
		}
	}
	return id;
}

// starts at 0
static int get_next_name_server_id(jdns_session_t *s)
{
	int n, id;
	id = -1;
	while(id == -1)
	{
		id = _int_wrap(&s->next_name_server_id, 0);
		for(n = 0; n < s->name_servers->count; ++n)
		{
			if(((name_server_t *)s->name_servers->item[n])->id == id)
			{
				id = -1;
				break;
			}
		}
	}
	return id;
}

int jdns_init_unicast(jdns_session_t *s, const jdns_address_t *addr, int port)
{
	int ret;
	s->mode = 0;
	ret = s->cb.udp_bind(s, s->cb.app, addr, port, 0);
	if(ret <= 0)
		return 0;
	s->handle = ret;
	s->port = port;
	return 1;
}

int jdns_init_multicast(jdns_session_t *s, const jdns_address_t *addr, int port, const jdns_address_t *maddr)
{
	int ret;
	s->mode = 1;
	ret = s->cb.udp_bind(s, s->cb.app, addr, port, maddr);
	if(ret <= 0)
		return 0;
	s->handle = ret;
	s->port = port;
	s->maddr = jdns_address_copy(maddr);

	// class 1.  note: frame size is ignored by the jdns version of mdnsd
	s->mdns = mdnsd_new(0x0001, 1000, s->port, _callback_time_now, _callback_rand_int, s);
	return 1;
}

void jdns_shutdown(jdns_session_t *s)
{
	if(s->shutdown == 0)
		s->shutdown = 1; // request shutdown
}

void jdns_set_nameservers(jdns_session_t *s, const jdns_nameserverlist_t *nslist)
{
	int n, k;

	// removed?
	for(k = 0; k < s->name_servers->count; ++k)
	{
		name_server_t *ns = (name_server_t *)(s->name_servers->item[k]);
		int found = 0;
		for(n = 0; n < nslist->count; ++n)
		{
			jdns_nameserver_t *i = (jdns_nameserver_t *)nslist->item[n];
			if(jdns_address_cmp(ns->address, i->address) && ns->port == i->port)
			{
				found = 1;
				break;
			}
		}
		if(!found)
		{
			int i;
			int ns_id;

			// remove any pending packets to this nameserver
			_remove_name_server_datagrams(s, ns->id);

			_debug_line(s, "ns [%s:%d] (id=%d) removed", ns->address->c_str, ns->port, ns->id);
			ns_id = ns->id;
			list_remove(s->name_servers, ns);
			--k; // adjust position
			for(i = 0; i < s->queries->count; ++i)
				query_name_server_gone((query_t *)s->queries->item[i], ns_id);
		}
	}

	// added?
	for(n = 0; n < nslist->count; ++n)
	{
		name_server_t *ns;
		jdns_nameserver_t *i;
		int found;

		i = (jdns_nameserver_t *)nslist->item[n];
		found = 0;
		for(k = 0; k < s->name_servers->count; ++k)
		{
			ns = (name_server_t *)(s->name_servers->item[k]);
			if(jdns_address_cmp(ns->address, i->address) && ns->port == i->port)
			{
				found = 1;
				break;
			}
		}
		if(found)
		{
			_debug_line(s, "ns [%s:%d] (id=%d) still present", ns->address->c_str, ns->port, ns->id);
		}
		else
		{
			ns = name_server_new();
			ns->id = get_next_name_server_id(s);
			ns->address = jdns_address_copy(i->address);
			ns->port = i->port;
			list_insert(s->name_servers, ns, -1);
			_debug_line(s, "ns [%s:%d] (id=%d) added", ns->address->c_str, ns->port, ns->id);
		}
	}

	// no nameservers?
	if(nslist->count == 0)
	{
		_debug_line(s, "nameserver count is zero, invalidating any queries");

		// invalidate all of the queries!
		for(n = 0; n < s->queries->count; ++n)
		{
			query_t *q = (query_t *)s->queries->item[n];

			// report event to any requests listening
			for(k = 0; k < q->req_ids_count; ++k)
			{
				jdns_event_t *event = jdns_event_new();
				event->type = JDNS_EVENT_RESPONSE;
				event->id = q->req_ids[k];
				event->status = JDNS_STATUS_TIMEOUT;
				_append_event_and_hold_id(s, event);
			}

			// this line is probably redundant, but just for
			//  consistency we'll do it...
			_remove_query_datagrams(s, q);

			list_remove(s->queries, q);
			--n; // adjust position
		}
	}
}

void jdns_probe(jdns_session_t *s)
{
	if(s->mode != 1)
		return;

	_multicast_flush(s);
}

int jdns_query(jdns_session_t *s, const unsigned char *name, int rtype)
{
	if(s->mode == 0)
		return _unicast_query(s, name, rtype);
	else
		return _multicast_query(s, name, rtype);
}

static void _remove_events(jdns_session_t *s, int event_type, int id)
{
	int n;
	for(n = 0; n < s->events->count; ++n)
	{
		event_t *e = (event_t *)s->events->item[n];
		if(e->event->type == event_type && e->event->id == id)
		{
			list_remove(s->events, e);
			--n; // adjust position
		}
	}
}

void jdns_cancel_query(jdns_session_t *s, int id)
{
	int n;

	_unhold_req_id(s, id);

	// remove any events associated with the query.  this avoids any
	//   possibility that stale events from one query are mistaken to be
	//   events resulting from a later query that happened to reuse the
	//   id.  it also means we don't deliver events for cancelled queries,
	//   which can simplify application logic.
	_remove_events(s, JDNS_EVENT_RESPONSE, id);

	// multicast
	if(s->mode == 1)
	{
		_multicast_cancel(s, id);
		return;
	}

	// unicast
	for(n = 0; n < s->queries->count; ++n)
	{
		query_t *q = (query_t *)s->queries->item[n];
		if(query_have_req_id(q, id))
		{
			query_remove_req_id(q, id);

			// note: calling _unicast_cancel might remove an item
			//  from s->queries, thereby screwing up our iterator
			//  position, but that's ok because we just break
			//  anyway.

			// if no one else is depending on this request, then take action
			if(q->req_ids_count == 0 && !q->cname_parent)
			{
				// remove a possible cname child
				if(q->cname_child && q->cname_child->req_ids_count == 0)
				{
					q->cname_child->cname_parent = 0;
					_unicast_cancel(s, q->cname_child);
					q->cname_child = 0;
				}

				_unicast_cancel(s, q);
			}
			break;
		}
	}
}

int jdns_publish(jdns_session_t *s, int mode, const jdns_rr_t *rr)
{
	return _multicast_publish(s, mode, rr);
}

void jdns_update_publish(jdns_session_t *s, int id, const jdns_rr_t *rr)
{
	_multicast_update_publish(s, id, rr);
}

void jdns_cancel_publish(jdns_session_t *s, int id)
{
	_unhold_req_id(s, id);

	_remove_events(s, JDNS_EVENT_PUBLISH, id);

	_multicast_cancel_publish(s, id);
}

int jdns_step(jdns_session_t *s)
{
	int now, passed;
	int ret;

	// session is shut down
	if(s->shutdown == 2)
		return 0;

	now = s->cb.time_now(s, s->cb.app);
	passed = now - s->last_time;

	_debug_line(s, "passed: %d", passed);

	if(s->mode == 0)
		ret = jdns_step_unicast(s, now);
	else
		ret = jdns_step_multicast(s, now);

	s->last_time = now;
	return ret;
}

int jdns_next_timer(jdns_session_t *s)
{
	return s->next_timer;
}

void jdns_set_handle_readable(jdns_session_t *s, int handle)
{
	(void)handle;
	s->handle_readable = 1;
}

void jdns_set_handle_writable(jdns_session_t *s, int handle)
{
	(void)handle;
	s->handle_writable = 1;
}

jdns_event_t *jdns_next_event(jdns_session_t *s)
{
	jdns_event_t *event = 0;
	if(s->events->count > 0)
	{
		event_t *e = (event_t *)s->events->item[0];
		event = e->event;
		e->event = 0;
		list_remove(s->events, e);
	}
	return event;
}

void jdns_set_hold_ids_enabled(jdns_session_t *s, int enabled)
{
	_set_hold_ids_enabled(s, enabled);
}

//----------------------------------------------------------------------------
// jdns - internal functions
//----------------------------------------------------------------------------

// we don't have vsnprintf on windows, so don't pass anything enormous to
//   this function.  the plan is that no line should exceed 1000 bytes,
//   although _print_rr() might get close.  a 2048 byte buffer should be
//   plenty then.
void _debug_line(jdns_session_t *s, const char *format, ...)
{
	char *buf = (char *)malloc(2048);
	va_list ap;
	va_start(ap, format);
	jdns_vsprintf_s(buf, 2048, format, ap);
	va_end(ap);
	s->cb.debug_line(s, s->cb.app, buf);
	free(buf);
}

int _callback_time_now(mdnsd d, void *arg)
{
	jdns_session_t *s = (jdns_session_t *)arg;
	(void)d;
	// offset the time, mdnsd doesn't like starting at 0
	return s->cb.time_now(s, s->cb.app) + 120 * 1000;
}

int _callback_rand_int(mdnsd d, void *arg)
{
	jdns_session_t *s = (jdns_session_t *)arg;
	(void)d;
	return s->cb.rand_int(s, s->cb.app);
}

void _append_event(jdns_session_t *s, jdns_event_t *event)
{
	event_t *e = event_new();
	e->event = event;
	list_insert(s->events, e, -1);
}

void _append_event_and_hold_id(jdns_session_t *s, jdns_event_t *event)
{
	if(s->do_hold_req_ids)
		_hold_req_id(s, event->id);
	_append_event(s, event);
}

void _remove_name_server_datagrams(jdns_session_t *s, int ns_id)
{
	int n;
	for(n = 0; n < s->outgoing->count; ++n)
	{
		datagram_t *a = (datagram_t *)s->outgoing->item[n];
		if(a->ns_id == ns_id)
		{
			list_remove(s->outgoing, a);
			--n; // adjust position
		}
	}
}

void _remove_query_datagrams(jdns_session_t *s, const query_t *q)
{
	int n;
	for(n = 0; n < s->outgoing->count; ++n)
	{
		datagram_t *a = (datagram_t *)s->outgoing->item[n];
		if(a->query == q)
		{
			list_remove(s->outgoing, a);
			--n; // adjust position
		}
	}
}

void _process_message(jdns_session_t *s, jdns_packet_t *p, int now, query_t *q, name_server_t *ns);

// return 1 if 'q' should be deleted, 0 if not
int _process_response(jdns_session_t *s, jdns_response_t *r, int nxdomain, int now, query_t *q);

jdns_response_t *_cache_get_response(jdns_session_t *s, const unsigned char *qname, int qtype, int *_lowest_timeleft)
{
	int n;
	int lowest_timeleft = -1;
	int now = s->cb.time_now(s, s->cb.app);
	jdns_response_t *r = 0;
	for(n = 0; n < s->cache->count; ++n)
	{
		cache_item_t *i = (cache_item_t *)s->cache->item[n];
		if(jdns_domain_cmp(i->qname, qname) && i->qtype == qtype)
		{
			int passed, timeleft;

			if(!r)
				r = jdns_response_new();

			if(i->record)
				jdns_response_append_answer(r, i->record);

			passed = now - i->time_start;
			timeleft = (i->ttl * 1000) - passed;
			if(lowest_timeleft == -1 || timeleft < lowest_timeleft)
				lowest_timeleft = timeleft;
		}
	}
	if(_lowest_timeleft)
		*_lowest_timeleft = lowest_timeleft;
	return r;
}

query_t *_find_first_active_query(jdns_session_t *s, const unsigned char *qname, int qtype)
{
	int n;
	query_t *q;

	for(n = 0; n < s->queries->count; ++n)
	{
		q = (query_t *)s->queries->item[n];
		if(jdns_domain_cmp(q->qname, qname) && q->qtype == qtype && q->step != -1)
			return q;
	}

	return 0;
}

query_t *_get_query(jdns_session_t *s, const unsigned char *qname, int qtype, int unique)
{
	query_t *q;
	jdns_string_t *str;

	if(!unique)
	{
		q = _find_first_active_query(s, qname, qtype);
		if(q)
		{
			str = _make_printable_cstr((const char *)q->qname);
			_debug_line(s, "[%d] reusing query for: [%s] [%s]", q->id, _qtype2str(qtype), str->data);
			jdns_string_delete(str);
			return q;
		}
	}

	q = query_new();
	q->id = get_next_qid(s);
	q->qname = _ustrdup(qname);
	q->qtype = qtype;
	q->step = 0;
	q->dns_id = -1;
	q->time_start = 0;
	q->time_next = 0;
	q->trycache = 1;
	q->retrying = 0;
	list_insert(s->queries, q, -1);

	str = _make_printable_cstr((const char *)q->qname);
	_debug_line(s, "[%d] querying: [%s] [%s]", q->id, _qtype2str(qtype), str->data);
	jdns_string_delete(str);
	return q;
}

int _unicast_query(jdns_session_t *s, const unsigned char *name, int qtype)
{
	unsigned char *qname;
	query_t *q;
	int req_id;
	jdns_string_t *str;

	str = _make_printable_cstr((const char *)name);
	_debug_line(s, "query input: [%s]", str->data);
	jdns_string_delete(str);

	qname = _fix_input(name);

	q = _get_query(s, qname, qtype, 0);
	req_id = get_next_req_id(s);
	query_add_req_id(q, req_id);
	free(qname);
	return req_id;
}

void _unicast_cancel(jdns_session_t *s, query_t *q)
{
	// didn't even do a step yet?  just remove it
	if(q->step == 0)
	{
		_remove_query_datagrams(s, q);
		list_remove(s->queries, q);
	}
	// otherwise, just deactivate
	else
	{
		// deactivate and remain in the background for
		//  1 minute.  this will allow us to cache a
		//  reply, even if the user is not currently
		//  interested.
		q->step = -1;
		q->time_start = s->cb.time_now(s, s->cb.app);
		q->time_next = 60000;
	}
}

void _queue_packet(jdns_session_t *s, query_t *q, const name_server_t *ns, int recurse, int query_send_type)
{
	jdns_packet_t *packet;
	datagram_t *a;

	packet = jdns_packet_new();
	packet->id = q->dns_id;
	packet->opts.rd = recurse; // recursion desired
	{
		jdns_packet_question_t *question = jdns_packet_question_new();
		question->qname = jdns_string_new();
		jdns_string_set_cstr(question->qname, (const char *)q->qname);
		question->qtype = q->qtype;
		question->qclass = 0x0001;
		jdns_list_insert(packet->questions, question, -1);
		jdns_packet_question_delete(question);
	}
	if(!jdns_packet_export(packet, JDNS_UDP_UNI_OUT_MAX))
	{
		_debug_line(s, "outgoing packet export error, not sending");
		jdns_packet_delete(packet);
		return;
	}

	a = datagram_new();
	a->handle = s->handle;
	a->dest_address = jdns_address_copy(ns->address);
	a->dest_port = ns->port;
	a->data = jdns_copy_array(packet->raw_data, packet->raw_size);
	a->size = packet->raw_size;
	a->query = q;
	a->query_send_type = query_send_type;
	a->ns_id = ns->id;

	jdns_packet_delete(packet);

	list_insert(s->outgoing, a, -1);
}

// return 1 if packets still need to be written
int _unicast_do_writes(jdns_session_t *s, int now);

// return 1 if packets still need to be read
int _unicast_do_reads(jdns_session_t *s, int now);

int jdns_step_unicast(jdns_session_t *s, int now)
{
	int n;
	int need_read = 0;
	int need_write = 0;
	int smallest_time = -1;
	int flags;

	if(s->shutdown == 1)
	{
		jdns_event_t *event = jdns_event_new();
		event->type = JDNS_EVENT_SHUTDOWN;
		_append_event(s, event);
		s->shutdown = 2;
		return 0;
	}

	// expire cached items
	for(n = 0; n < s->cache->count; ++n)
	{
		cache_item_t *i = (cache_item_t *)s->cache->item[n];
		if(now >= i->time_start + (i->ttl * 1000))
		{
			jdns_string_t *str = _make_printable_cstr((const char *)i->qname);
			_debug_line(s, "cache exp [%s]", str->data);
			jdns_string_delete(str);
			list_remove(s->cache, i);
			--n; // adjust position
		}
	}

	need_write = _unicast_do_writes(s, now);
	need_read = _unicast_do_reads(s, now);

	// calculate next timer (based on queries and cache)
	for(n = 0; n < s->queries->count; ++n)
	{
		query_t *q = (query_t *)(s->queries->item[n]);
		if(q->time_start != -1)
		{
			int qpassed = now - q->time_start;
			int timeleft = q->time_next - qpassed;
			if(timeleft < 0)
				timeleft = 0;

			if(smallest_time == -1 || timeleft < smallest_time)
				smallest_time = timeleft;
		}
	}
	for(n = 0; n < s->cache->count; ++n)
	{
		cache_item_t *i = (cache_item_t *)(s->cache->item[n]);
		int passed = now - i->time_start;
		int timeleft = (i->ttl * 1000) - passed;
		if(timeleft < 0)
			timeleft = 0;

		if(smallest_time == -1 || timeleft < smallest_time)
			smallest_time = timeleft;
	}

	flags = 0;
	if(smallest_time != -1)
	{
		flags |= JDNS_STEP_TIMER;
		s->next_timer = smallest_time;

		// offset it a little bit, so that the user doesn't call
		//  us too early, resulting in a no-op and another timer
		//  of 1 millisecond.
		s->next_timer += 2;
	}
	if(need_read || need_write)
		flags |= JDNS_STEP_HANDLE;
	return flags;
}

int _unicast_do_writes(jdns_session_t *s, int now)
{
	int need_write = 0;
	int n, k;

	for(n = 0; n < s->queries->count; ++n)
	{
		query_t *q;
		int qpassed, timeleft;
		int giveup;
		name_server_t *ns;
		int already_sending;

		q = (query_t *)s->queries->item[n];

		// nothing to do
		if(q->time_start == -1)
			continue;

		qpassed = now - q->time_start;
		timeleft = q->time_next - qpassed;
		if(timeleft < 0)
			timeleft = 0;
		_debug_line(s, "[%d] time_start/next=%d/%d (left=%d)", q->id, q->time_start, q->time_next, timeleft);
		if(timeleft > 0)
			continue;

		if(q->trycache)
		{
			// is it cached?
			int lowest_timeleft;
			int qtype = q->qtype;
			jdns_response_t *r;

			r = _cache_get_response(s, q->qname, qtype, &lowest_timeleft);

			// not found?  try cname
			if(!r)
			{
				qtype = JDNS_RTYPE_CNAME;
				r = _cache_get_response(s, q->qname, qtype, &lowest_timeleft);
			}

			if(r)
			{
				int nxdomain;

				_debug_line(s, "[%d] using cached answer", q->id);

				// are any of the records about to expire in 3 minutes?
				//  assume the client is interested in this record and
				//  query it again "in the background" (but only
				//  if we are not already doing so)
				if(lowest_timeleft < (3 * 60 * 1000) && !_find_first_active_query(s, q->qname, q->qtype))
				{
					query_t *new_q;

					_debug_line(s, "requerying for cached item about to expire");

					new_q = _get_query(s, q->qname, q->qtype, 1);
					new_q->retrying = 1; // slow it down
					new_q->trycache = 0; // don't use the cache for this
				}

				nxdomain = r->answerCount == 0 ? 1 : 0;
				if(_process_response(s, r, nxdomain, -1, q))
				{
					_remove_query_datagrams(s, q);
					list_remove(s->queries, q);
					--n; // adjust position
				}

				jdns_response_delete(r);
				continue;
			}
		}

		// inactive
		if(q->step == -1)
		{
			// time up on an inactive query?  remove it
			_debug_line(s, "removing inactive query");
			_remove_query_datagrams(s, q);
			list_remove(s->queries, q);
			--n; // adjust position
			continue;
		}

		giveup = 0;

		// too many tries, give up
		if(q->step == 8)
			giveup = 1;

		// no nameservers, give up
		//  (this would happen if someone removed all nameservers
		//   during a query)
		if(s->name_servers->count == 0)
			giveup = 1;

		if(giveup)
		{
			// report event to any requests listening
			for(k = 0; k < q->req_ids_count; ++k)
			{
				jdns_event_t *event = jdns_event_new();
				event->type = JDNS_EVENT_RESPONSE;
				event->id = q->req_ids[k];
				event->status = JDNS_STATUS_TIMEOUT;
				_append_event_and_hold_id(s, event);
			}

			// report error to parent
			if(q->cname_parent)
			{
				// report event to any requests listening
				query_t *cq = q->cname_parent;
				for(k = 0; k < cq->req_ids_count; ++k)
				{
					jdns_event_t *event = jdns_event_new();
					event->type = JDNS_EVENT_RESPONSE;
					event->id = cq->req_ids[k];
					event->status = JDNS_STATUS_TIMEOUT;
					_append_event_and_hold_id(s, event);
				}
				list_remove(s->queries, cq);
			}

			_remove_query_datagrams(s, q);
			list_remove(s->queries, q);
			--n; // adjust position
			continue;
		}

		// assign a packet id if we don't have one yet
		if(q->dns_id == -1)
		{
			q->dns_id = get_next_dns_id(s);

			// couldn't get an id?
			if(q->dns_id == -1)
			{
				_debug_line(s, "unable to reserve packet id");

				// report event to any requests listening
				for(k = 0; k < q->req_ids_count; ++k)
				{
					jdns_event_t *event = jdns_event_new();
					event->type = JDNS_EVENT_RESPONSE;
					event->id = q->req_ids[k];
					event->status = JDNS_STATUS_ERROR;
					_append_event_and_hold_id(s, event);
				}

				// report error to parent
				if(q->cname_parent)
				{
					// report event to any requests listening
					query_t *cq = q->cname_parent;
					for(k = 0; k < cq->req_ids_count; ++k)
					{
						jdns_event_t *event = jdns_event_new();
						event->type = JDNS_EVENT_RESPONSE;
						event->id = cq->req_ids[k];
						event->status = JDNS_STATUS_ERROR;
						_append_event_and_hold_id(s, event);
					}
					list_remove(s->queries, cq);
				}

				_remove_query_datagrams(s, q);
				list_remove(s->queries, q);
				--n; // adjust position
				continue;
			}
		}

		// out of name servers?
		if(q->servers_tried_count == s->name_servers->count)
		{
			// clear the 'tried' list, and start over in retry mode
			query_clear_servers_tried(q);
			q->retrying = 1;
		}

		// find a nameserver that has not been tried
		ns = 0;
		for(k = 0; k < s->name_servers->count; ++k)
		{
			name_server_t *i = (name_server_t *)s->name_servers->item[k];
			if(!query_server_tried(q, i->id))
			{
				ns = i;
				break;
			}
		}

		// in theory, it is not possible for 'ns' to be null here

		// don't send the packet if there is already one in the queue
		already_sending = 0;
		for(k = 0; k < s->outgoing->count; ++k)
		{
			datagram_t *a = (datagram_t *)s->outgoing->item[k];
			if(a->query == q && a->query_send_type == 0)
			{
				already_sending = 1;
				break;
			}
		}

		// send the query, with recursion desired, normal query_send_type
		if(!already_sending)
			_queue_packet(s, q, ns, 1, 0);

		query_add_server_tried(q, ns->id);

		// if there is one query, then do a trick on the first step
		/*if(s->queries->count == 1 && q->step == 0 && !q->retrying)
		{
			// query all other servers non-recursively
			// note: if sending fails, there is no retry
			for(k = 0; k < s->name_servers->count; ++k)
			{
				name_server_t *i = (name_server_t *)s->name_servers->item[k];
				if(!query_server_tried(q, i->id))
				{
					// last arg sets first-step query_send_type
					_queue_packet(s, q, i, 0, 1);
				}
			}
		}*/

		// out of name servers?
		if(q->servers_tried_count == s->name_servers->count)
		{
			// clear the 'tried' list, and start over in retry mode
			query_clear_servers_tried(q);
			q->retrying = 1;
		}

		q->time_start = now;
		q->time_next = q->retrying ? 1500 : 800;
		++q->step;
	}

	// try to send queued outgoing packets
	for(n = 0; n < s->outgoing->count; ++n)
	{
		datagram_t *a = (datagram_t *)s->outgoing->item[n];
		int ret;

		if(!s->handle_writable)
		{
			need_write = 1;
			break;
		}

		_debug_line(s, "SEND %s:%d (size=%d)", a->dest_address->c_str, a->dest_port, a->size);
		_print_hexdump(s, a->data, a->size);

		ret = s->cb.udp_write(s, s->cb.app, a->handle, a->dest_address, a->dest_port, a->data, a->size);
		if(ret == 0)
		{
			s->handle_writable = 0;
			need_write = 1;
			break;
		}

		list_remove(s->outgoing, a);
		--n; // adjust position
	}

	return need_write;
}

void _cache_add(jdns_session_t *s, const unsigned char *qname, int qtype, int time_start, int ttl, const jdns_rr_t *record)
{
	cache_item_t *i;
	jdns_string_t *str;
	if(ttl == 0)
		return;
	if(s->cache->count >= JDNS_CACHE_MAX)
		return;
	i = cache_item_new();
	i->qname = _ustrdup(qname);
	i->qtype = qtype;
	i->time_start = time_start;
	i->ttl = ttl;
	if(record)
		i->record = jdns_rr_copy(record);
	list_insert(s->cache, i, -1);

	str = _make_printable_cstr((const char *)i->qname);
	_debug_line(s, "cache add [%s] for %d seconds", str->data, i->ttl);
	jdns_string_delete(str);
}

void _cache_remove_all_of_kind(jdns_session_t *s, const unsigned char *qname, int qtype)
{
	int n;
	for(n = 0; n < s->cache->count; ++n)
	{
		cache_item_t *i = (cache_item_t *)s->cache->item[n];
		if(jdns_domain_cmp(i->qname, qname) && i->qtype == qtype)
		{
			jdns_string_t *str = _make_printable_cstr((const char *)i->qname);
			_debug_line(s, "cache del [%s]", str->data);
			jdns_string_delete(str);
			list_remove(s->cache, i);
			--n; // adjust position
		}
	}
}

void _cache_remove_all_of_record(jdns_session_t *s, const jdns_rr_t *record)
{
	int n;
	for(n = 0; n < s->cache->count; ++n)
	{
		cache_item_t *i = (cache_item_t *)s->cache->item[n];
		if(i->record && _cmp_rr(i->record, record))
		{
			jdns_string_t *str = _make_printable_cstr((const char *)i->qname);
			_debug_line(s, "cache del [%s]", str->data);
			jdns_string_delete(str);
			list_remove(s->cache, i);
			--n; // adjust position
		}
	}
}

// same as _cache_add, but make sure the exact same record (name AND value)
//   isn't stored twice, and make sure no more than one cname record per name
//   is stored.
void _cache_add_no_dups(jdns_session_t *s, const unsigned char *qname, int qtype, int time_start, int ttl, const jdns_rr_t *record)
{
	if(qtype == JDNS_RTYPE_CNAME)
		_cache_remove_all_of_kind(s, qname, qtype);
	else
		_cache_remove_all_of_record(s, record);

	_cache_add(s, qname, qtype, time_start, ttl, record);
}

int _unicast_do_reads(jdns_session_t *s, int now)
{
	int need_read;
	int n, k;

	// let's always ask for reads, just so the user doesn't have to
	//  worry about what should happen to incoming packets otherwise
	need_read = 1;

	if(!s->handle_readable)
		return need_read;

	while(1)
	{
		unsigned char buf[JDNS_UDP_UNI_IN_MAX];
		int bufsize = JDNS_UDP_UNI_IN_MAX;
		int ret;
		jdns_packet_t *packet;
		jdns_address_t *addr;
		int port;
		query_t *q;
		name_server_t *ns;

		addr = jdns_address_new();
		ret = s->cb.udp_read(s, s->cb.app, s->handle, addr, &port, buf, &bufsize);

		// no packet?
		if(ret == 0)
		{
			s->handle_readable = 0;
			jdns_address_delete(addr);
			break;
		}

		_debug_line(s, "RECV %s:%d (size=%d)", addr->c_str, port, bufsize);
		_print_hexdump(s, buf, bufsize);

		if(!jdns_packet_import(&packet, buf, bufsize))
		{
			_debug_line(s, "error parsing packet / too large");

			jdns_address_delete(addr);
			continue;
		}

		_print_packet(s, packet);

		if(s->queries->count == 0)
		{
			_debug_line(s, "we have no queries");

			jdns_address_delete(addr);
			jdns_packet_delete(packet);
			continue;
		}

		// who does it belong to?
		q = 0;
		ns = 0;
		for(n = 0; n < s->queries->count; ++n)
		{
			query_t *i = (query_t *)s->queries->item[n];
			if(i->dns_id == -1)
				continue;

			if(i->dns_id == packet->id)
			{
				q = i;
				break;
			}
		}

		if(q)
		{
			// what name server did it come from?
			for(k = 0; k < s->name_servers->count; ++k)
			{
				name_server_t *i = (name_server_t *)s->name_servers->item[k];
				if(jdns_address_cmp(i->address, addr) && i->port == port)
				{
					ns = i;
					break;
				}
			}

			// none? maybe that's because we're using unicast
			//   over multicast, where responses always come
			//   from an unexpected address
			if(!ns && s->name_servers->count > 0)
			{
				name_server_t *i;
				jdns_address_t *m4, *m6;

				i = (name_server_t *)s->name_servers->item[0];
				m4 = jdns_address_multicast4_new();
				m6 = jdns_address_multicast6_new();
				if(jdns_address_cmp(i->address, m4) || jdns_address_cmp(i->address, m6))
					ns = i;
				jdns_address_delete(m4);
				jdns_address_delete(m6);
			}

			// no suitable name server
			if(!ns)
			{
				// setting q = 0 causes the response to be
				//   ignored.  earlier versions of jdns would
				//   do this, but now we comment it out because
				//   the behavior is too strict.
				//q = 0;

				// instead we'll just print a warning
				_debug_line(s, "warning: response from unexpected nameserver");
			}
		}

		jdns_address_delete(addr);

		// no queries?  eat the packet
		if(!q)
		{
			_debug_line(s, "no such query for packet");
			jdns_packet_delete(packet);
			continue;
		}

		_process_message(s, packet, now, q, ns);
		jdns_packet_delete(packet);
	}

	return need_read;
}

void _process_message(jdns_session_t *s, jdns_packet_t *packet, int now, query_t *q, name_server_t *ns)
{
	int n;
	int authoritative;
	int truncated;
	int recursion_desired;
	int answer_section_ok;
	jdns_response_t *r;

	if(packet->opts.opcode != 0)
	{
		_debug_line(s, "opcode != 0, discarding");
		return;
	}

	// we don't test RA (recursion available)
	// we don't test the extra Z fields

	authoritative = packet->opts.aa;
	truncated = packet->opts.tc;
	recursion_desired = packet->opts.rd;
	answer_section_ok = 0;
	if(packet->qdcount == packet->questions->count && packet->ancount == packet->answerRecords->count)
		answer_section_ok = 1;

	r = 0;

	// nxdomain
	if(packet->opts.rcode == 3)
	{
		// treat nxdomain as a generic error, but at the same time flag
		//   the fact that it was received.  this ensures that
		//   resolving keeps going, in case the user has multiple dns
		//   servers and one of them reports nxdomain when a later one
		//   would succeed.  if all of the servers fail then this flag
		//   can be used at the end to report nxdomain instead of a
		//   generic error.
		q->nxdomain = 1;
	}
	// normal
	else if(packet->opts.rcode == 0)
	{
		int at_least_something;
		int success;

		r = _packet2response(packet, q->qname, q->qtype, 0xffff);
		at_least_something = 0;
		if(r->answerCount > 0)
			at_least_something = 1;
		_print_records(s, r, q->qname);

		success = 0;
		if(at_least_something)
		{
			success = 1;
		}
		else
		{
			// note: why does qdns care about recursion_desired here?
			if(authoritative && recursion_desired)
				success = 1;
		}

		if(!success)
		{
			jdns_response_delete(r);
			r = 0;
		}
	}

	// caching
	if(r)
	{
		int cache_answers;
		int cache_additional;

		// clear past items
		_cache_remove_all_of_kind(s, q->qname, q->qtype);

		cache_answers = 1;
		cache_additional = 1;

		// if truncated, we may not want to cache
		if(truncated)
		{
			cache_additional = 0;
			if(!answer_section_ok)
				cache_answers = 0;
		}

		if(cache_answers)
		{
			for(n = 0; n < r->answerCount; ++n)
			{
				jdns_rr_t *record = r->answerRecords[n];
				_cache_add_no_dups(s, q->qname, record->type, now, _min(record->ttl, JDNS_TTL_MAX), record);
			}
		}

		if(cache_additional)
		{
			for(n = 0; n < r->additionalCount; ++n)
			{
				jdns_rr_t *record = r->additionalRecords[n];
				_cache_add_no_dups(s, record->owner, record->type, now, _min(record->ttl, JDNS_TTL_MAX), record);
			}
		}
	}

	// don't pass authority/additional records upwards
	if(r)
		jdns_response_remove_extra(r);

	// this server returned an error?
	if(!r && ns)
	{
		// all failed servers must also be considered tried servers,
		//   so mark as tried if necessary.  this can happen if the
		//   tried list is cleared (to perform retrying) and then an
		//   error is received
		if(!query_server_tried(q, ns->id))
			query_add_server_tried(q, ns->id);

		query_add_server_failed(q, ns->id);
	}

	if(_process_response(s, r, 0, now, q))
	{
		_remove_query_datagrams(s, q);
		list_remove(s->queries, q);
	}

	jdns_response_delete(r);
}

// 'r' can be null, for processing an error
// 'now' can be -1, if processing a cached response ('r' always non-null)
int _process_response(jdns_session_t *s, jdns_response_t *r, int nxdomain, int now, query_t *q)
{
	int k;
	int do_error = 0;
	int do_nxdomain = 0;

	// error
	if(!r)
	{
		int all_errored;

		// if not all servers have errored, ignore error
		all_errored = 1;
		for(k = 0; k < s->name_servers->count; ++k)
		{
			name_server_t *ns = (name_server_t *)s->name_servers->item[k];
			if(!query_server_failed(q, ns->id))
			{
				all_errored = 0;
				break;
			}
		}
		if(!all_errored)
			return 0;

		do_error = 1;

		// if we picked up an nxdomain along the way, act on it now
		if(q->nxdomain)
		{
			do_nxdomain = 1;

			// cache nxdomain for 1 minute
			if(q->qtype != JDNS_RTYPE_ANY && now != -1)
			{
				_cache_remove_all_of_kind(s, q->qname, q->qtype);
				_cache_add(s, q->qname, q->qtype, now, 60, 0);
			}
		}
	}
	else if(nxdomain)
	{
		do_error = 1;
		do_nxdomain = 1;
	}

	if(do_error)
	{
		// report event to any requests listening
		for(k = 0; k < q->req_ids_count; ++k)
		{
			jdns_event_t *event = jdns_event_new();
			event->type = JDNS_EVENT_RESPONSE;
			event->id = q->req_ids[k];
			if(do_nxdomain)
				event->status = JDNS_STATUS_NXDOMAIN;
			else
				event->status = JDNS_STATUS_ERROR;
			_append_event_and_hold_id(s, event);
		}

		// report error to parent
		if(q->cname_parent)
		{
			// report event to any requests listening
			query_t *cq = q->cname_parent;
			for(k = 0; k < cq->req_ids_count; ++k)
			{
				jdns_event_t *event = jdns_event_new();
				event->type = JDNS_EVENT_RESPONSE;
				event->id = cq->req_ids[k];
				event->status = JDNS_STATUS_ERROR;
				_append_event_and_hold_id(s, event);
			}
			list_remove(s->queries, cq);
		}

		return 1;
	}

	// all we got was a cname that we didn't ask for?
	if(r->answerCount == 1 && r->answerRecords[0]->type == JDNS_RTYPE_CNAME && q->qtype != JDNS_RTYPE_CNAME)
	{
		query_t *new_q;

		_debug_line(s, "all we got was a cname, following the chain ...");

		// max chain count, bail
		if(q->cname_chain_count >= JDNS_CNAME_MAX)
		{
			// report event to any requests listening
			for(k = 0; k < q->req_ids_count; ++k)
			{
				jdns_event_t *event = jdns_event_new();
				event->type = JDNS_EVENT_RESPONSE;
				event->id = q->req_ids[k];
				event->status = JDNS_STATUS_ERROR;
				_append_event_and_hold_id(s, event);
			}

			// report error to parent
			if(q->cname_parent)
			{
				// report event to any requests listening
				query_t *cq = q->cname_parent;
				for(k = 0; k < cq->req_ids_count; ++k)
				{
					jdns_event_t *event = jdns_event_new();
					event->type = JDNS_EVENT_RESPONSE;
					event->id = cq->req_ids[k];
					event->status = JDNS_STATUS_ERROR;
					_append_event_and_hold_id(s, event);
				}
				list_remove(s->queries, cq);
			}

			return 1;
		}

		new_q = _get_query(s, r->answerRecords[0]->data.name, q->qtype, 1);

		// is the current query a child query? (has a parent)
		if(q->cname_parent)
		{
			// if so, then set new_q as the new child
			new_q->cname_chain_count = q->cname_chain_count + 1;
			new_q->cname_parent = q->cname_parent;
			new_q->cname_parent->cname_child = new_q;

			// and delete the current query
			return 1;
		}
		else
		{
			// otherwise, the current query becomes a parent, with
			//   new_q set as the child
			new_q->cname_chain_count = q->cname_chain_count + 1;
			new_q->cname_parent = q;
			q->cname_child = new_q;
			q->time_start = -1;
			q->dns_id = -1; // don't handle responses
		}
	}

	// if this query now has a child, then don't report events or delete
	if(q->cname_child)
		return 0;

	// report event to any requests listening
	for(k = 0; k < q->req_ids_count; ++k)
	{
		jdns_event_t *event = jdns_event_new();
		event->type = JDNS_EVENT_RESPONSE;
		event->id = q->req_ids[k];
		event->status = JDNS_STATUS_SUCCESS;
		event->response = jdns_response_copy(r);
		_append_event_and_hold_id(s, event);
	}

	// report to parent
	if(q->cname_parent)
	{
		// report event to any requests listening
		query_t *cq = q->cname_parent;
		for(k = 0; k < cq->req_ids_count; ++k)
		{
			jdns_event_t *event = jdns_event_new();
			event->type = JDNS_EVENT_RESPONSE;
			event->id = cq->req_ids[k];
			event->status = JDNS_STATUS_SUCCESS;
			event->response = jdns_response_copy(r);
			_append_event_and_hold_id(s, event);
		}
		list_remove(s->queries, cq);
	}

	return 1;
}

//----------------------------------------------------------------------------
// jdns - multicast
//----------------------------------------------------------------------------
static jdns_rr_t *_mdnsda2rr(mdnsda a)
{
	jdns_rr_t *rr;

	if(a->type == JDNS_RTYPE_ANY)
		return 0;

	// for AAAA, TXT and HINFO, run the raw rdata through jdns_rr's parser
	if(a->type == JDNS_RTYPE_AAAA || a->type == JDNS_RTYPE_TXT || a->type == JDNS_RTYPE_HINFO)
	{
		jdns_packet_resource_t *pr = jdns_packet_resource_new();
		pr->qname = jdns_string_new();
		jdns_string_set_cstr(pr->qname, (const char *)a->name);
		pr->qtype = a->type;
		pr->qclass = 0x0001; // class is always 1 for us
		if(a->ttl == 0)
			pr->ttl = 0;
		else
			pr->ttl = a->real_ttl;
		pr->rdata = jdns_copy_array(a->rdata, a->rdlen);
		pr->rdlength = a->rdlen;

		// we don't need a reference for these types
		rr = jdns_rr_from_resource(pr, 0);
	}
	// else, pull the values out of 'a' directly
	else
	{
		rr = jdns_rr_new();
		rr->owner = _ustrdup(a->name);
		rr->qclass = 0x0001; // class is always 1 for us
		if(a->ttl == 0)
			rr->ttl = 0;
		else
			rr->ttl = a->real_ttl;

		switch(a->type)
		{
			case JDNS_RTYPE_A:
			{
				jdns_address_t *addr = jdns_address_new();
				jdns_address_set_ipv4(addr, a->ip);
				jdns_rr_set_A(rr, addr);
				jdns_address_delete(addr);
				break;
			}
			case JDNS_RTYPE_AAAA:
			{
				// covered earlier
				break;
			}
			case JDNS_RTYPE_MX:
			{
				// don't care about MX
				jdns_rr_delete(rr);
				rr = 0;
				break;
			}
			case JDNS_RTYPE_SRV:
			{
				jdns_rr_set_SRV(rr, a->rdname, a->srv.port, a->srv.priority, a->srv.weight);
				break;
			}
			case JDNS_RTYPE_CNAME:
			{
				jdns_rr_set_CNAME(rr, a->rdname);
				break;
			}
			case JDNS_RTYPE_PTR:
			{
				jdns_rr_set_PTR(rr, a->rdname);
				break;
			}
			case JDNS_RTYPE_TXT:
			{
				// covered earlier
				break;
			}
			case JDNS_RTYPE_HINFO:
			{
				// covered earlier
				break;
			}
			case JDNS_RTYPE_NS:
			{
				// don't care about NS
				jdns_rr_delete(rr);
				rr = 0;
				break;
			}
			default:
			{
				jdns_rr_set_record(rr, a->type, a->rdata, a->rdlen);
				break;
			}
		}
	}

	return rr;
}

int _multicast_query_ans(mdnsda a, void *arg)
{
	int n;
	jdns_session_t *s;
	query_t *q;
	jdns_response_t *r;
	jdns_rr_t *rr;
	jdns_event_t *event;

	s = (jdns_session_t *)arg;

	// what query is this for?
	q = 0;
	for(n = 0; n < s->queries->count; ++n)
	{
		query_t *i = (query_t *)s->queries->item[n];
		if((i->qtype == JDNS_RTYPE_ANY || i->qtype == a->type) && jdns_domain_cmp(i->qname, a->name))
		{
			q = i;
			break;
		}
	}

	// note: this can't happen, but we'll check anyway
	if(!q)
	{
		_debug_line(s, "no such multicast query");
		return 0;
	}

	rr = _mdnsda2rr(a);
	if(!rr)
		return 0;

	// add/remove as a known
	if(rr->ttl == 0)
	{
		for(n = 0; n < q->mul_known->answerCount; ++n)
		{
			jdns_rr_t *k = q->mul_known->answerRecords[n];
			if(_cmp_rr(k, rr))
			{
				jdns_response_remove_answer(q->mul_known, n);
				break;
			}
		}
	}
	else
		jdns_response_append_answer(q->mul_known, rr);

	r = jdns_response_new();
	jdns_response_append_answer(r, rr);
	jdns_rr_delete(rr);

	// report event to any requests listening
	for(n = 0; n < q->req_ids_count; ++n)
	{
		event = jdns_event_new();
		event->type = JDNS_EVENT_RESPONSE;
		event->id = q->req_ids[n];
		event->status = JDNS_STATUS_SUCCESS;
		event->response = jdns_response_copy(r);
		_append_event(s, event);
	}

	jdns_response_delete(r);
	return 0;
}

query_t *_get_multicast_query(jdns_session_t *s, const unsigned char *qname, int qtype)
{
	int n;
	query_t *q;
	jdns_string_t *str;

	// check for existing queries
	for(n = 0; n < s->queries->count; ++n)
	{
		q = (query_t *)s->queries->item[n];
		if(jdns_domain_cmp(q->qname, qname) && q->qtype == qtype)
		{
			str = _make_printable_cstr((const char *)q->qname);
			_debug_line(s, "[%d] reusing query for: [%s] [%s]", q->id, _qtype2str(qtype), str->data);
			jdns_string_delete(str);
			return q;
		}
	}

	q = query_new();
	q->id = get_next_qid(s);
	q->qname = _ustrdup(qname);
	q->qtype = qtype;
	q->step = 0;
	q->mul_known = jdns_response_new();
	list_insert(s->queries, q, -1);

	str = _make_printable_cstr((const char *)q->qname);
	_debug_line(s, "[%d] querying: [%s] [%s]", q->id, _qtype2str(qtype), str->data);
	jdns_string_delete(str);
	return q;
}

int _multicast_query(jdns_session_t *s, const unsigned char *name, int qtype)
{
	unsigned char *qname;
	query_t *q;
	int req_id;
	jdns_string_t *str;

	str = _make_printable_cstr((const char *)name);
	_debug_line(s, "query input: [%s]", str->data);
	jdns_string_delete(str);

	// add a dot to the end if needed
	qname = _fix_input(name);

	q = _get_multicast_query(s, qname, qtype);
	req_id = get_next_req_id(s);
	query_add_req_id(q, req_id);
	free(qname);

	// start the mdnsd_query if necessary
	if(q->step == 0)
	{
		q->step = 1;
		mdnsd_query(s->mdns, (char *)q->qname, q->qtype, _multicast_query_ans, s);
	}
	else
	{
		int n;

		// report the knowns
		for(n = 0; n < q->mul_known->answerCount; ++n)
		{
			const jdns_rr_t *rr;
			jdns_response_t *r;
			jdns_event_t *event;

			rr = q->mul_known->answerRecords[n];
			r = jdns_response_new();
			jdns_response_append_answer(r, rr);

			event = jdns_event_new();
			event->type = JDNS_EVENT_RESPONSE;
			event->id = req_id;
			event->status = JDNS_STATUS_SUCCESS;
			event->response = r;
			_append_event(s, event);
		}
	}
	return req_id;
}

void _multicast_cancel(jdns_session_t *s, int req_id)
{
	int n;
	for(n = 0; n < s->queries->count; ++n)
	{
		query_t *q = (query_t *)s->queries->item[n];
		if(query_have_req_id(q, req_id))
		{
			query_remove_req_id(q, req_id);

			// if no one else is depending on this request, then take action
			if(q->req_ids_count == 0)
			{
				mdnsd_query(s->mdns, (char *)q->qname, q->qtype, NULL, 0);
				list_remove(s->queries, q);
			}
			break;
		}
	}
}

void _multicast_pubresult(int result, char *name, int type, void *arg)
{
	jdns_session_t *s;
	published_item_t *pub;
	jdns_event_t *event;
	int n;

	s = (jdns_session_t *)arg;

	// find the associated pub item
	pub = 0;
	for(n = 0; n < s->published->count; ++n)
	{
		published_item_t *i = (published_item_t *)s->published->item[n];
		if(strcmp((char *)i->qname, name) == 0 && i->qtype == type)
		{
			pub = i;
			break;
		}
	}

	// note: this can't happen, but we'll check anyway
	if(!pub)
	{
		_debug_line(s, "no such multicast published item");
		return;
	}

	if(result == 1)
	{
		jdns_string_t *str = _make_printable_cstr(name);
		_debug_line(s, "published name %s for type %d", str->data, type);
		jdns_string_delete(str);

		event = jdns_event_new();
		event->type = JDNS_EVENT_PUBLISH;
		event->id = pub->id;
		event->status = JDNS_STATUS_SUCCESS;
		_append_event(s, event);
	}
	else
	{
		jdns_string_t *str = _make_printable_cstr(name);
		_debug_line(s, "conflicting name detected %s for type %d", str->data, type);
		jdns_string_delete(str);

		event = jdns_event_new();
		event->type = JDNS_EVENT_PUBLISH;
		event->id = pub->id;
		event->status = JDNS_STATUS_CONFLICT;
		_append_event_and_hold_id(s, event);

		// remove the item
		list_remove(s->published, pub);
	}
}

static jdns_string_t *_create_text(const jdns_stringlist_t *texts)
{
	jdns_string_t *out;
	int n;
	int total;
	unsigned char *buf;

	buf = 0;
	total = 0;
	for(n = 0; n < texts->count; ++n)
		total += texts->item[n]->size + 1;
	if(total > 0)
	{
		int at = 0;
		buf = (unsigned char *)malloc(total);
		for(n = 0; n < texts->count; ++n)
		{
			unsigned int len = texts->item[n]->size;
			buf[at++] = len;
			memcpy(buf + at, texts->item[n]->data, len);
			at += len;
		}
	}

	out = jdns_string_new();
	if(buf)
	{
		out->data = buf;
		out->size = total;
	}
	else
		jdns_string_set_cstr(out, "");
	return out;
}

static void _publish_applyrr_unknown(jdns_session_t *s, mdnsdr r, const jdns_rr_t *rr)
{
	// for unsupported/unknown, just take the rdata
	// note: for this to work, the app must explicitly set the rdata.
	//   if the record is MX or some other known but unsupported record
	//   type, setting the known fields is not enough
	mdnsd_set_raw(s->mdns, r, (char *)rr->rdata, rr->rdlength);
}

static int _publish_applyrr(jdns_session_t *s, mdnsdr r, const jdns_rr_t *rr)
{
	if(!rr->haveKnown)
	{
		_publish_applyrr_unknown(s, r, rr);
		return 1;
	}

	// jdns_mdnsd supports: A, AAAA, SRV, CNAME, PTR, TXT, and HINFO
	switch(rr->type)
	{
		case JDNS_RTYPE_A:
		{
			unsigned long int ip_net = htonl(rr->data.address->addr.v4);
			mdnsd_set_raw(s->mdns, r, (char *)&ip_net, 4);
			break;
		}
		case JDNS_RTYPE_AAAA:
		{
			mdnsd_set_raw(s->mdns, r, (char *)rr->data.address->addr.v6, 16);
			break;
		}
		case JDNS_RTYPE_SRV:
		{
			mdnsd_set_srv(s->mdns, r, rr->data.server->priority, rr->data.server->weight, rr->data.server->port, (char *)rr->data.server->name);
			break;
		}
		case JDNS_RTYPE_CNAME:
		{
			mdnsd_set_host(s->mdns, r, (char *)rr->data.name);
			break;
		}
		case JDNS_RTYPE_PTR:
		{
			mdnsd_set_host(s->mdns, r, (char *)rr->data.name);
			break;
		}
		case JDNS_RTYPE_TXT:
		{
			jdns_string_t *out = _create_text(rr->data.texts);
			mdnsd_set_raw(s->mdns, r, (char *)out->data, out->size);
			jdns_string_delete(out);
			break;
		}
		case JDNS_RTYPE_HINFO:
		{
			jdns_string_t *out;
			jdns_stringlist_t *list;

			list = jdns_stringlist_new();
			jdns_stringlist_append(list, rr->data.hinfo.cpu);
			jdns_stringlist_append(list, rr->data.hinfo.os);
			out = _create_text(list);
			jdns_stringlist_delete(list);

			mdnsd_set_raw(s->mdns, r, (char *)out->data, out->size);
			jdns_string_delete(out);
			break;
		}
		default:
		{
			_publish_applyrr_unknown(s, r, rr);
			break;
		}
	}

	return 1;
}

static void report_published(jdns_session_t *s, published_item_t *pub)
{
	jdns_event_t *event;
	jdns_string_t *str;

	str = _make_printable_cstr((char *)pub->qname);
	_debug_line(s, "published name %s for type %d", str->data, pub->qtype);
	jdns_string_delete(str);

	event = jdns_event_new();
	event->type = JDNS_EVENT_PUBLISH;
	event->id = pub->id;
	event->status = JDNS_STATUS_SUCCESS;
	_append_event(s, event);
}

int _multicast_publish(jdns_session_t *s, int mode, const jdns_rr_t *rr)
{
	mdnsdr r;
	published_item_t *pub;
	int next_id;
	jdns_event_t *event;
	int n;

	r = 0;
	next_id = get_next_req_id(s);

	// see if we have an item with this name+type combination already
	pub = 0;
	for(n = 0; n < s->published->count; ++n)
	{
		published_item_t *i = (published_item_t *)s->published->item[n];
		if(i->qtype == rr->type && jdns_domain_cmp(i->qname, rr->owner))
		{
			pub = i;
			break;
		}
	}
	if(pub)
		goto error;

	if(!jdns_rr_verify(rr))
		goto error;

	if(mode == JDNS_PUBLISH_UNIQUE)
		r = mdnsd_unique(s->mdns, (char *)rr->owner, rr->type, rr->ttl, _multicast_pubresult, s);
	else
		r = mdnsd_shared(s->mdns, (char *)rr->owner, rr->type, rr->ttl);

	if(!_publish_applyrr(s, r, rr))
		goto error;

	pub = published_item_new();
	pub->id = next_id;
	pub->mode = mode;
	pub->qname = _ustrdup(rr->owner);
	pub->qtype = rr->type;
	pub->rec = r;
	pub->rr = jdns_rr_copy(rr);
	list_insert(s->published, pub, -1);

	// mdnsd doesn't report publish events for shared, so do that here
	if(mode == JDNS_PUBLISH_SHARED)
		report_published(s, pub);

	return pub->id;

error:
	_debug_line(s, "attempt to publish record, malformed, unsupported, or duplicate type");

	if(r)
	{
		// don't publish
		mdnsd_done(s->mdns, r);
	}

	// send an error to the app
	event = jdns_event_new();
	event->type = JDNS_EVENT_PUBLISH;
	event->id = next_id;
	event->status = JDNS_STATUS_ERROR;
	_append_event_and_hold_id(s, event);

	return next_id;
}

void _multicast_update_publish(jdns_session_t *s, int id, const jdns_rr_t *rr)
{
	mdnsdr r;
	published_item_t *pub;
	int qtype;
	int n;

	pub = 0;
	for(n = 0; n < s->published->count; ++n)
	{
		published_item_t *i = (published_item_t *)s->published->item[n];
		if(i->id == id)
		{
			pub = i;
			break;
		}
	}
	if(!pub)
		return;

	qtype = pub->qtype;
	r = pub->rec;

	// expire existing record.  this is mostly needed for shared records
	//   since unique records already have the cache flush bit and that
	//   should achieve the same result.  however, since Apple expires
	//   unique records before updates, so will we.
	mdnsd_done(s->mdns, r);
	if(pub->mode == JDNS_PUBLISH_UNIQUE)
		r = mdnsd_unique(s->mdns, (char *)pub->rr->owner, pub->rr->type, rr->ttl, _multicast_pubresult, s);
	else
		r = mdnsd_shared(s->mdns, (char *)pub->rr->owner, pub->rr->type, rr->ttl);
	pub->rec = r;

	if(!_publish_applyrr(s, r, rr))
	{
		_debug_line(s, "attempt to update_publish an unsupported type");
		return;
	}
}

void _multicast_cancel_publish(jdns_session_t *s, int id)
{
	int n;
	for(n = 0; n < s->published->count; ++n)
	{
		published_item_t *i = (published_item_t *)s->published->item[n];
		if(i->id == id)
		{
			mdnsd_done(s->mdns, i->rec);
			list_remove(s->published, i);
			break;
		}
	}
}

void _multicast_flush(jdns_session_t *s)
{
	int n;

	// to flush, we make like our queries and published items are all new.
	// we'll do this by destroying/creating the mdnsd object again (so it
	// is fresh) and then reapply all queries and published items to it.

	// start over with mdnsd
	mdnsd_free(s->mdns);
	s->mdns = mdnsd_new(0x0001, 1000, s->port, _callback_time_now, _callback_rand_int, s);

	// attempt to publish again
	for(n = 0; n < s->published->count; ++n)
	{
		published_item_t *i;
		mdnsdr r;

		i = (published_item_t *)s->published->item[n];
		if(i->mode == JDNS_PUBLISH_UNIQUE)
			r = mdnsd_unique(s->mdns, (char *)i->rr->owner, i->rr->type, i->rr->ttl, _multicast_pubresult, s);
		else
			r = mdnsd_shared(s->mdns, (char *)i->rr->owner, i->rr->type, i->rr->ttl);
		_publish_applyrr(s, r, i->rr);
		i->rec = r;
	}

	// restore the queries
	for(n = 0; n < s->queries->count; ++n)
	{
		query_t *q = (query_t *)s->queries->item[n];

		// issue the query
		mdnsd_query(s->mdns, (char *)q->qname, q->qtype, _multicast_query_ans, s);
	}
}

int jdns_step_multicast(jdns_session_t *s, int now)
{
	int need_read, need_write, smallest_time;
	struct mytimeval *tv;
	jdns_packet_t *packet;
	int flags;

	// not used
	(void)now;

	need_read = 0;
	need_write = 0;

	if(s->shutdown == 1)
		mdnsd_shutdown(s->mdns);

	while(1)
	{
		jdns_address_t *addr;
		unsigned short int port;
		int ret;
		unsigned char *buf;
		int buf_len;

		if(!mdnsd_out(s->mdns, &packet, &addr, &port))
			break;

		if(!s->handle_writable)
		{
			need_write = 1;
			jdns_address_delete(addr);
			break;
		}

		if(!jdns_packet_export(packet, JDNS_UDP_MUL_OUT_MAX))
		{
			_debug_line(s, "outgoing packet export error, not sending");
			jdns_packet_delete(packet);
			continue;
		}

		buf = packet->raw_data;
		buf_len = packet->raw_size;

		// multicast
		if(!addr)
		{
			addr = jdns_address_copy(s->maddr);
			port = s->port;
		}

		_debug_line(s, "SEND %s:%d (size=%d)", addr->c_str, port, buf_len);
		_print_hexdump(s, buf, buf_len);

		ret = s->cb.udp_write(s, s->cb.app, s->handle, addr, port, buf, buf_len);

		jdns_address_delete(addr);
		jdns_packet_delete(packet);

		// if we can't write the packet, oh well
		if(ret == 0)
		{
			s->handle_writable = 0;
			need_write = 1;
			break;
		}
	}

	if(s->shutdown == 1)
	{
		jdns_event_t *event = jdns_event_new();
		event->type = JDNS_EVENT_SHUTDOWN;
		_append_event(s, event);
		s->shutdown = 2;
		return 0;
	}

	// let's always ask for reads, just so the user doesn't have to
	//  worry about what should happen to incoming packets otherwise
	need_read = 1;

	if(s->handle_readable)
	{
		while(1)
		{
			unsigned char buf[JDNS_UDP_MUL_IN_MAX];
			int bufsize = JDNS_UDP_MUL_IN_MAX;
			int ret;
			jdns_address_t *addr;
			int port;
			jdns_response_t *r;

			addr = jdns_address_new();
			ret = s->cb.udp_read(s, s->cb.app, s->handle, addr, &port, buf, &bufsize);

			// no packet?
			if(ret == 0)
			{
				s->handle_readable = 0;
				jdns_address_delete(addr);
				break;
			}

			_debug_line(s, "RECV %s:%d (size=%d)", addr->c_str, port, bufsize);
			_print_hexdump(s, buf, bufsize);

			if(!jdns_packet_import(&packet, buf, bufsize))
			{
				_debug_line(s, "error parsing packet / too large");

				jdns_address_delete(addr);
				continue;
			}

			_print_packet(s, packet);

			r = _packet2response(packet, 0, 0, 0x7fff);
			_print_records(s, r, 0);

			mdnsd_in(s->mdns, packet, r, addr, (unsigned short)port);

			jdns_address_delete(addr);
			jdns_packet_delete(packet);
			jdns_response_delete(r);
		}
	}

	tv = mdnsd_sleep(s->mdns);
	smallest_time = tv->tv_sec * 1000 + tv->tv_usec / 1000;

	flags = 0;
	if(smallest_time != -1)
	{
		flags |= JDNS_STEP_TIMER;
		s->next_timer = smallest_time;

		// offset it a little bit, so that the user doesn't call
		//  us too early, resulting in a no-op and another timer
		//  of 1 millisecond.
		s->next_timer += 2;
	}
	if(need_read || need_write)
		flags |= JDNS_STEP_HANDLE;
	return flags;
}
