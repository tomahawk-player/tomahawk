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

#include "jdns_packet.h"

//----------------------------------------------------------------------------
// misc
//----------------------------------------------------------------------------
void *jdns_alloc(int size)
{
	return malloc(size);
}

void *jdns_realloc(void *p, int size)
{
	return realloc(p, size);
}

void jdns_free(void *p)
{
	free(p);
}

char *jdns_strdup(const char *s)
{
	char *p;
	int len;

	len = strlen(s) + 1; // the zero
	p = (char *)jdns_alloc(len);
	memcpy(p, s, len);
	return p;
}

unsigned char *jdns_copy_array(const unsigned char *src, int size)
{
	unsigned char *out;
	if(size <= 0)
		return 0;
	out = (unsigned char *)jdns_alloc(size);
	memcpy(out, src, size);
	return out;
}

int jdns_domain_cmp(const unsigned char *a, const unsigned char *b)
{
	int n;
	int len_a;

	// case-insensitive compare
	len_a = _ustrlen(a);
	if(len_a != (int)_ustrlen(b))
		return 0;

	for(n = 0; n < len_a; ++n)
	{
		if(tolower(a[n]) != tolower(b[n]))
			return 0;
	}
	return 1;
}

int jdns_sprintf_s(char *str, int n, const char *format, ...)
{
	int ret;
	va_list ap;
	va_start(ap, format);
	ret = jdns_vsprintf_s(str, n, format, ap);
	va_end(ap);
	return ret;
}

int jdns_vsprintf_s(char *str, int n, const char *format, va_list ap)
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
	return vsprintf_s(str, n, format, ap);
#else
	(void)n;
	return vsprintf(str, format, ap);
#endif
}

FILE *jdns_fopen(const char *path, const char *mode)
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
	FILE *fp;
	if(fopen_s(&fp, path, mode) != 0)
		return 0;
	return fp;
#else
	return fopen(path, mode);
#endif
}

jdns_string_t *jdns_getenv(const char *name)
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
	jdns_string_t *out;
	char *dest;
	size_t size;
	int sizei;
	errno_t ret;
	ret = getenv_s(&size, 0, 0, name);
	if(ret != 0 || size == 0)
		return 0;
	sizei = (int)size;
	dest = (char *)jdns_alloc(sizei);
	ret = getenv_s(&size, dest, size, name);
	if(ret != 0)
	{
		free(dest);
		return 0;
	}
	out = jdns_string_new();
	out->size = sizei - 1;
	out->data = dest; // must be zero-terminated, which it is
	return out;
#else
	jdns_string_t *out;
	char *val;
	val = getenv(name);
	if(!val)
		return 0;
	out = jdns_string_new();
	jdns_string_set_cstr(out, val);
	return out;
#endif
}

char *jdns_strcpy(char *dst, const char *src)
{
#if defined(_MSC_VER) && _MSC_VER >= 1400
	int len;
	// deliberately unsafe
	len = strlen(src);
	if(strcpy_s(dst, len + 1, src) != 0)
		return 0;
	return dst;
#else
	return strcpy(dst, src);
#endif
}

//----------------------------------------------------------------------------
// jdns_object
//----------------------------------------------------------------------------
void *jdns_object_new(int size, void (*dtor)(void *), void *(*cctor)(const void *))
{
	jdns_object_t *a = (jdns_object_t *)jdns_alloc(size);
	memset(a, 0, size);
	a->dtor = dtor;
	a->cctor = cctor;
	return a;
}

void *jdns_object_copy(const void *a)
{
	return ((const jdns_object_t *)a)->cctor(a);
}

void jdns_object_delete(void *a)
{
	((jdns_object_t *)a)->dtor(a);
}

void jdns_object_free(void *a)
{
	jdns_free(a);
}

//----------------------------------------------------------------------------
// jdns_list
//----------------------------------------------------------------------------
jdns_list_t *jdns_list_new()
{
	jdns_list_t *a = JDNS_OBJECT_NEW(jdns_list);
	a->count = 0;
	a->item = 0;
	a->valueList = 0;
	a->autoDelete = 0;
	return a;
}

jdns_list_t *jdns_list_copy(const jdns_list_t *a)
{
	jdns_list_t *c = jdns_list_new();

	// note: copying a list with autoDelete should not ever be done.
	//   heck, let's not even allow it.  return an empty list.
	if(a->autoDelete)
		return c;

	c->valueList = a->valueList;

	// copy the items
	if(a->item)
	{
		int n;
		c->count = a->count;
		c->item = (void **)jdns_alloc(sizeof(void *) * c->count);
		if(a->valueList)
		{
			// deep copy
			for(n = 0; n < c->count; ++n)
				c->item[n] = jdns_object_copy(a->item[n]);
		}
		else
		{
			// just the pointer
			for(n = 0; n < c->count; ++n)
				c->item[n] = a->item[n];
		}
	}
	return c;
}

void jdns_list_delete(jdns_list_t *a)
{
	if(!a)
		return;
	jdns_list_clear(a);
	jdns_object_free(a);
}

void jdns_list_clear(jdns_list_t *a)
{
	if(a->item)
	{
		// delete the items if necessary
		if(a->valueList || a->autoDelete)
		{
			int n;
			for(n = 0; n < a->count; ++n)
				jdns_object_delete(a->item[n]);
		}
		jdns_free(a->item);
		a->item = 0;
		a->count = 0;
	}
}

void jdns_list_insert(jdns_list_t *a, void *item, int pos)
{
	// make memory
	if(!a->item)
		a->item = (void **)jdns_alloc(sizeof(void *));
	else
		a->item = (void **)jdns_realloc(a->item, sizeof(void *) * (a->count + 1));

	// prepare position
	if(pos != -1)
		memmove(a->item + pos + 1, a->item + pos, (a->count - pos) * sizeof(void *));
	else
		pos = a->count;

	// insert it
	if(a->valueList)
		a->item[pos] = jdns_object_copy(item);
	else
		a->item[pos] = item;
	++a->count;
}

void jdns_list_insert_value(jdns_list_t *a, const void *item, int pos)
{
	jdns_list_insert(a, (void *)item, pos);
}

void jdns_list_remove(jdns_list_t *a, void *item)
{
	int n;
	int pos = -1;
	for(n = 0; n < a->count; ++n)
	{
		if(a->item[n] == item)
		{
			pos = n;
			break;
		}
	}
	if(pos == -1)
		return;

	jdns_list_remove_at(a, pos);
}

void jdns_list_remove_at(jdns_list_t *a, int pos)
{
	if(pos < 0 || pos >= a->count)
		return;

	// delete the item if necessary
	if(a->valueList || a->autoDelete)
		jdns_object_delete(a->item[pos]);

	// free the position
	if(a->count > 1)
	{
		memmove(a->item + pos, a->item + pos + 1, (a->count - pos - 1) * sizeof(void *));
		--a->count;
	}
	else
	{
		jdns_free(a->item);
		a->item = 0;
		a->count = 0;
	}
}

//----------------------------------------------------------------------------
// jdns_string
//----------------------------------------------------------------------------
jdns_string_t *jdns_string_new()
{
	jdns_string_t *s = JDNS_OBJECT_NEW(jdns_string);
	s->data = 0;
	s->size = 0;
	return s;
}

jdns_string_t *jdns_string_copy(const jdns_string_t *s)
{
	jdns_string_t *c = jdns_string_new();
	if(s->data)
		jdns_string_set(c, s->data, s->size);
	return c;
}

void jdns_string_delete(jdns_string_t *s)
{
	if(!s)
		return;
	if(s->data)
		jdns_free(s->data);
	jdns_object_free(s);
}

void jdns_string_set(jdns_string_t *s, const unsigned char *str, int str_len)
{
	if(s->data)
		jdns_free(s->data);
	s->data = (unsigned char *)jdns_alloc(str_len + 1);
	memcpy(s->data, str, str_len);
	s->data[str_len] = 0;
	s->size = str_len;
}

void jdns_string_set_cstr(jdns_string_t *s, const char *str)
{
	jdns_string_set(s, (const unsigned char *)str, strlen(str));
}

int jdns_string_indexOf(const jdns_string_t *s, unsigned char c, int pos)
{
	int n;
	for(n = pos; n < s->size; ++n)
	{
		if(s->data[n] == c)
			return n;
	}
	return -1;
}

jdns_stringlist_t *jdns_string_split(const jdns_string_t *s, unsigned char sep)
{
	int at, n, len;
	jdns_string_t *str;
	jdns_stringlist_t *out;

	at = 0;
	out = jdns_stringlist_new();
	while(at < s->size)
	{
		n = jdns_string_indexOf(s, sep, at);
		if(n == -1)
			n = s->size;
		len = n - at;
		// FIXME: should we allow empty items?
		//if(len == 0)
		//	break;
		str = jdns_string_new();
		jdns_string_set(str, s->data + at, len);
		jdns_stringlist_append(out, str);
		jdns_string_delete(str);
		at = n + 1; // skip over separator
	}
	return out;
}

//----------------------------------------------------------------------------
// jdns_stringlist
//----------------------------------------------------------------------------
jdns_stringlist_t *jdns_stringlist_new()
{
	jdns_list_t *a = jdns_list_new();
	a->valueList = 1;
	return (jdns_stringlist_t *)a;
}

jdns_stringlist_t *jdns_stringlist_copy(const jdns_stringlist_t *a)
{
	return (jdns_stringlist_t *)jdns_list_copy((const jdns_list_t *)a);
}

void jdns_stringlist_delete(jdns_stringlist_t *a)
{
	jdns_list_delete((jdns_list_t *)a);
	// note: no need to call jdns_object_free() here
}

void jdns_stringlist_append(jdns_stringlist_t *a, const jdns_string_t *str)
{
	jdns_list_insert_value((jdns_list_t *)a, str, -1);
}

//----------------------------------------------------------------------------
// jdns_address
//----------------------------------------------------------------------------
jdns_address_t *jdns_address_new()
{
	jdns_address_t *a = alloc_type(jdns_address_t);
	a->isIpv6 = 0;
	a->addr.v4 = 0;
	a->c_str = jdns_strdup("");
	return a;
}

jdns_address_t *jdns_address_copy(const jdns_address_t *a)
{
	jdns_address_t *c = jdns_address_new();
	if(a->isIpv6)
		jdns_address_set_ipv6(c, a->addr.v6);
	else
		jdns_address_set_ipv4(c, a->addr.v4);
	return c;
}

void jdns_address_delete(jdns_address_t *a)
{
	if(!a)
		return;
	if(a->isIpv6)
		jdns_free(a->addr.v6);
	jdns_free(a->c_str);
	jdns_free(a);
}

void jdns_address_set_ipv4(jdns_address_t *a, unsigned long int ipv4)
{
	if(a->isIpv6)
		jdns_free(a->addr.v6);
	jdns_free(a->c_str);
	a->isIpv6 = 0;
	a->addr.v4 = ipv4;
	a->c_str = (char *)jdns_alloc(16); // max size (3 * 4 + 3 + 1)
	jdns_sprintf_s(a->c_str, 16, "%d.%d.%d.%d",
		(unsigned char)((ipv4 >> 24) & 0xff),
		(unsigned char)((ipv4 >> 16) & 0xff),
		(unsigned char)((ipv4 >>  8) & 0xff),
		(unsigned char)((ipv4)       & 0xff));
}

void jdns_address_set_ipv6(jdns_address_t *a, const unsigned char *ipv6)
{
	int n;
	unsigned char *p;
	unsigned short word[8];
	if(a->isIpv6)
		jdns_free(a->addr.v6);
	jdns_free(a->c_str);
	a->isIpv6 = 1;
	a->addr.v6 = (unsigned char *)jdns_alloc(16);
	memcpy(a->addr.v6, ipv6, 16);
	p = (unsigned char *)a->addr.v6;
	a->c_str = (char *)jdns_alloc(40); // max size (8 * 4 + 7 + 1)
	// each word in a 16-byte ipv6 address is network byte order
	for(n = 0; n < 8; ++n)
		word[n] = ((unsigned short)(p[n * 2]) << 8) + (unsigned short)(p[n * 2 + 1]);
	jdns_sprintf_s(a->c_str, 40, "%04X:%04X:%04X:%04X:%04X:%04X:%04X:%04X", word[0], word[1], word[2], word[3], word[4], word[5], word[6], word[7]);
}

int jdns_address_set_cstr(jdns_address_t *a, const char *str)
{
	int slen = strlen(str);

	// ipv6
	if(strchr(str, ':'))
	{
		jdns_string_t *in;
		jdns_stringlist_t *list;
		unsigned char ipv6[16];
		int n, at, count, fill;

		in = jdns_string_new();
		jdns_string_set_cstr(in, str);
		list = jdns_string_split(in, ':');
		jdns_string_delete(in);

		// a confusing outputting-backwards parser adapted from qt

		count = list->count;

		if(count < 3 || count > 8)
			goto error;

		at = 16;
		fill = 9 - count;
		for(n = count - 1; n >= 0; --n)
		{
			if(at <= 0)
				goto error;

			if(list->item[n]->size == 0)
			{
				if(n == count - 1)
				{
					if(list->item[n - 1]->size != 0)
						goto error;
					ipv6[--at] = 0;
					ipv6[--at] = 0;
				}
				else if(n == 0)
				{
					if(list->item[n + 1]->size != 0)
						goto error;
					ipv6[--at] = 0;
					ipv6[--at] = 0;
				}
				else
				{
					int i;
					for(i = 0; i < fill; ++i)
					{
						if(at <= 0)
							goto error;
						ipv6[--at] = 0;
						ipv6[--at] = 0;
					}
				}
			}
			else
			{
				if(jdns_string_indexOf(list->item[n], '.', 0) == -1)
				{
					int x;
					x = strtol((const char *)list->item[n]->data, NULL, 16);
					if(x < 0 || x > 0xffff)
						goto error;
					ipv6[--at] = x & 0xff;
					ipv6[--at] = (x >> 8) & 0xff;
				}
				else
				{
					jdns_address_t *v4;

					if(n != count - 1)
						goto error;

					v4 = jdns_address_new();
					if(!jdns_address_set_cstr(v4, (char *)list->item[n]->data))
					{
						jdns_address_delete(v4);
						goto error;
					}

					ipv6[--at] = (unsigned char)(v4->addr.v4 & 0xff);
					ipv6[--at] = (unsigned char)((v4->addr.v4 >> 8) & 0xff);
					ipv6[--at] = (unsigned char)((v4->addr.v4 >> 16) & 0xff);
					ipv6[--at] = (unsigned char)((v4->addr.v4 >> 24) & 0xff);
					jdns_address_delete(v4);
					--fill;
				}
			}
		}
		jdns_stringlist_delete(list);

		jdns_address_set_ipv6(a, ipv6);
		return 1;

error:
		jdns_stringlist_delete(list);
		return 0;
	}
	else if(strchr(str, '.'))
	{
		unsigned char b[4];
		int x;
		unsigned long int ipv4;
		int at;
		char *part;
		int len;
		const char *p, *p2;

		p = str;
		at = 0;
		while(1)
		{
			p2 = strchr(p, '.');
			if(!p2)
				p2 = str + slen;
			len = p2 - p;

			// convert the section into a byte
			part = (char *)jdns_alloc(len + 1);
			memcpy(part, p, len);
			part[len] = 0;
			x = strtol(part, NULL, 10);
			jdns_free(part);
			if(x < 0 || x > 0xff)
				break;
			b[at++] = (unsigned char)x;

			// done?
			if(p2 >= str + slen)
				break;

			// skip over the separator
			p = p2 + 1;
		}
		if(at != 4)
			return 0;

		ipv4 = 0;
		ipv4 += b[0];
		ipv4 <<= 8;
		ipv4 += b[1];
		ipv4 <<= 8;
		ipv4 += b[2];
		ipv4 <<= 8;
		ipv4 += b[3];

		jdns_address_set_ipv4(a, ipv4);
		return 1;
	}
	else
		return 0;
}

int jdns_address_cmp(const jdns_address_t *a, const jdns_address_t *b)
{
	// same protocol?
	if(a->isIpv6 != b->isIpv6)
		return 0;
	if(a->isIpv6)
	{
		int n;
		for(n = 0; n < 16; ++n)
		{
			if(a->addr.v6[n] != b->addr.v6[n])
				break;
		}
		if(n == 16)
			return 1;
	}
	else
	{
		if(a->addr.v4 == b->addr.v4)
			return 1;
	}
	return 0;
}

// FF02::FB
unsigned char jdns_multicast_addr6_value_v6[] =
{
	0xff, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfb
};

jdns_address_t *jdns_address_multicast4_new()
{
	jdns_address_t *a = jdns_address_new();
	jdns_address_set_ipv4(a, 0xe00000fb);
	return a;
}

jdns_address_t *jdns_address_multicast6_new()
{
	jdns_address_t *a = jdns_address_new();
	jdns_address_set_ipv6(a, jdns_multicast_addr6_value_v6);
	return a;
}

//----------------------------------------------------------------------------
// jdns_server
//----------------------------------------------------------------------------
jdns_server_t *jdns_server_new()
{
	jdns_server_t *s = alloc_type(jdns_server_t);
	s->name = 0;
	s->port = 0;
	s->priority = 0;
	s->weight = 0;
	return s;
}

jdns_server_t *jdns_server_copy(const jdns_server_t *s)
{
	jdns_server_t *c = jdns_server_new();
	if(s->name)
		c->name = _ustrdup(s->name);
	c->port = s->port;
	c->priority = s->priority;
	c->weight = s->weight;
	return c;
}

void jdns_server_delete(jdns_server_t *s)
{
	if(!s)
		return;
	if(s->name)
		jdns_free(s->name);
	jdns_object_free(s);
}

void jdns_server_set_name(jdns_server_t *s, const unsigned char *name)
{
	if(s->name)
		jdns_free(s->name);
	s->name = _ustrdup(name);
}

//----------------------------------------------------------------------------
// jdns_nameserver
//----------------------------------------------------------------------------
jdns_nameserver_t *jdns_nameserver_new()
{
	jdns_nameserver_t *a = alloc_type(jdns_nameserver_t);
	a->address = 0;
	a->port = -1;
	return a;
}

jdns_nameserver_t *jdns_nameserver_copy(const jdns_nameserver_t *a)
{
	jdns_nameserver_t *c = jdns_nameserver_new();
	if(a->address)
		c->address = jdns_address_copy(a->address);
	c->port = a->port;
	return c;
}

void jdns_nameserver_delete(jdns_nameserver_t *a)
{
	if(!a)
		return;
	jdns_address_delete(a->address);
	jdns_free(a);
}

void jdns_nameserver_set(jdns_nameserver_t *a, const jdns_address_t *addr, int port)
{
	if(a->address)
		jdns_address_delete(a->address);
	a->address = jdns_address_copy(addr);
	a->port = port;
}

//----------------------------------------------------------------------------
// jdns_nameserverlist
//----------------------------------------------------------------------------
jdns_nameserverlist_t *jdns_nameserverlist_new()
{
	jdns_nameserverlist_t *a = alloc_type(jdns_nameserverlist_t);
	a->count = 0;
	a->item = 0;
	return a;
}

jdns_nameserverlist_t *jdns_nameserverlist_copy(const jdns_nameserverlist_t *a)
{
	int n;
	jdns_nameserverlist_t *c = jdns_nameserverlist_new();
	if(a->item)
	{
		c->item = (jdns_nameserver_t **)jdns_alloc(sizeof(jdns_nameserver_t *) * a->count);
		c->count = a->count;
		for(n = 0; n < c->count; ++n)
			c->item[n] = jdns_nameserver_copy(a->item[n]);
	}
	return c;
}

void jdns_nameserverlist_delete(jdns_nameserverlist_t *a)
{
	int n;
	if(!a)
		return;
	if(a->item)
	{
		for(n = 0; n < a->count; ++n)
			jdns_nameserver_delete(a->item[n]);
		jdns_free(a->item);
	}
	jdns_free(a);
}

void jdns_nameserverlist_append(jdns_nameserverlist_t *a, const jdns_address_t *addr, int port)
{
	if(!a->item)
		a->item = (jdns_nameserver_t **)jdns_alloc(sizeof(jdns_nameserver_t *));
	else
		a->item = (jdns_nameserver_t **)jdns_realloc(a->item, sizeof(jdns_nameserver_t *) * (a->count + 1));
	a->item[a->count] = jdns_nameserver_new();
	jdns_nameserver_set(a->item[a->count], addr, port);
	++a->count;
}

//----------------------------------------------------------------------------
// jdns_dnshost
//----------------------------------------------------------------------------
jdns_dnshost_t *jdns_dnshost_new()
{
	jdns_dnshost_t *a = alloc_type(jdns_dnshost_t);
	a->name = 0;
	a->address = 0;
	return a;
}

jdns_dnshost_t *jdns_dnshost_copy(const jdns_dnshost_t *a)
{
	jdns_dnshost_t *c = jdns_dnshost_new();
	if(a->name)
		c->name = jdns_string_copy(a->name);
	if(a->address)
		c->address = jdns_address_copy(a->address);
	return c;
}

void jdns_dnshost_delete(jdns_dnshost_t *a)
{
	if(!a)
		return;
	jdns_string_delete(a->name);
	jdns_address_delete(a->address);
	jdns_free(a);
}

//----------------------------------------------------------------------------
// jdns_dnshostlist
//----------------------------------------------------------------------------
jdns_dnshostlist_t *jdns_dnshostlist_new()
{
	jdns_dnshostlist_t *a = alloc_type(jdns_dnshostlist_t);
	a->count = 0;
	a->item = 0;
	return a;
}

jdns_dnshostlist_t *jdns_dnshostlist_copy(const jdns_dnshostlist_t *a)
{
	int n;
	jdns_dnshostlist_t *c = jdns_dnshostlist_new();
	if(a->item)
	{
		c->item = (jdns_dnshost_t **)jdns_alloc(sizeof(jdns_dnshost_t *) * a->count);
		c->count = a->count;
		for(n = 0; n < c->count; ++n)
			c->item[n] = jdns_dnshost_copy(a->item[n]);
	}
	return c;
}

void jdns_dnshostlist_delete(jdns_dnshostlist_t *a)
{
	int n;
	if(!a)
		return;
	if(a->item)
	{
		for(n = 0; n < a->count; ++n)
			jdns_dnshost_delete(a->item[n]);
		jdns_free(a->item);
	}
	jdns_free(a);
}

void jdns_dnshostlist_append(jdns_dnshostlist_t *a, const jdns_dnshost_t *host)
{
	if(!a->item)
		a->item = (jdns_dnshost_t **)jdns_alloc(sizeof(jdns_dnshost_t *));
	else
		a->item = (jdns_dnshost_t **)jdns_realloc(a->item, sizeof(jdns_dnshost_t *) * (a->count + 1));
	a->item[a->count] = jdns_dnshost_copy(host);
	++a->count;
}

//----------------------------------------------------------------------------
// jdns_dnsparams
//----------------------------------------------------------------------------
jdns_dnsparams_t *jdns_dnsparams_new()
{
	jdns_dnsparams_t *a = alloc_type(jdns_dnsparams_t);
	a->nameservers = jdns_nameserverlist_new();
	a->domains = jdns_stringlist_new();
	a->hosts = jdns_dnshostlist_new();
	return a;
}

jdns_dnsparams_t *jdns_dnsparams_copy(jdns_dnsparams_t *a)
{
	jdns_dnsparams_t *c = jdns_dnsparams_new();
	c->nameservers = jdns_nameserverlist_copy(a->nameservers);
	c->domains = jdns_stringlist_copy(a->domains);
	c->hosts = jdns_dnshostlist_copy(a->hosts);
	return c;
}

void jdns_dnsparams_delete(jdns_dnsparams_t *a)
{
	if(!a)
		return;
	jdns_nameserverlist_delete(a->nameservers);
	jdns_stringlist_delete(a->domains);
	jdns_dnshostlist_delete(a->hosts);
	jdns_free(a);
}

void jdns_dnsparams_append_nameserver(jdns_dnsparams_t *a, const jdns_address_t *addr, int port)
{
	jdns_nameserverlist_append(a->nameservers, addr, port);
}

void jdns_dnsparams_append_domain(jdns_dnsparams_t *a, const jdns_string_t *domain)
{
	jdns_stringlist_append(a->domains, domain);
}

void jdns_dnsparams_append_host(jdns_dnsparams_t *a, const jdns_string_t *name, const jdns_address_t *address)
{
	jdns_dnshost_t *h = jdns_dnshost_new();
	h->name = jdns_string_copy(name);
	h->address = jdns_address_copy(address);
	jdns_dnshostlist_append(a->hosts, h);
	jdns_dnshost_delete(h);
}

//----------------------------------------------------------------------------
// jdns_rr
//----------------------------------------------------------------------------
void _jdns_rr_data_reset(jdns_rr_t *r)
{
	if(r->rdata)
	{
		jdns_free(r->rdata);
		r->rdata = 0;
	}
	r->rdlength = 0;

	if(r->haveKnown)
	{
		switch(r->type)
		{
			case JDNS_RTYPE_A:
			case JDNS_RTYPE_AAAA:
				jdns_address_delete(r->data.address);
				break;
			case JDNS_RTYPE_MX:
			case JDNS_RTYPE_SRV:
				jdns_server_delete(r->data.server);
				break;
			case JDNS_RTYPE_CNAME:
			case JDNS_RTYPE_PTR:
			case JDNS_RTYPE_NS:
				jdns_free(r->data.name);
				break;
			case JDNS_RTYPE_TXT:
				jdns_stringlist_delete(r->data.texts);
				break;
			case JDNS_RTYPE_HINFO:
				jdns_string_delete(r->data.hinfo.cpu);
				jdns_string_delete(r->data.hinfo.os);
				break;
			default:
				break;
		};
		r->haveKnown = 0;
	}
	r->type = -1;
}

void _jdns_rr_data_copy(const jdns_rr_t *r, jdns_rr_t *c)
{
	c->type = r->type;
	c->qclass = r->qclass;
	c->rdlength = r->rdlength;
	c->rdata = jdns_copy_array(r->rdata, r->rdlength);

	if(r->haveKnown)
	{
		switch(r->type)
		{
			case JDNS_RTYPE_A:
			case JDNS_RTYPE_AAAA:
				c->data.address = jdns_address_copy(r->data.address);
				break;
			case JDNS_RTYPE_MX:
			case JDNS_RTYPE_SRV:
				c->data.server = jdns_server_copy(r->data.server);
				break;
			case JDNS_RTYPE_CNAME:
			case JDNS_RTYPE_PTR:
			case JDNS_RTYPE_NS:
				c->data.name = _ustrdup(r->data.name);
				break;
			case JDNS_RTYPE_TXT:
				c->data.texts = jdns_stringlist_copy(r->data.texts);
				break;
			case JDNS_RTYPE_HINFO:
				c->data.hinfo.cpu = jdns_string_copy(r->data.hinfo.cpu);
				c->data.hinfo.os = jdns_string_copy(r->data.hinfo.os);
				break;
			default:
				break;
		};
		c->haveKnown = 1;
	}
}

jdns_rr_t *jdns_rr_new()
{
	jdns_rr_t *r = alloc_type(jdns_rr_t);
	r->owner = 0;
	r->ttl = 0;
	r->type = -1;
	r->qclass = 0;
	r->rdata = 0;
	r->rdlength = 0;
	r->haveKnown = 0;
	return r;
}

jdns_rr_t *jdns_rr_copy(const jdns_rr_t *r)
{
	jdns_rr_t *c = jdns_rr_new();
	if(r->owner)
		c->owner = _ustrdup(r->owner);
	c->ttl = r->ttl;
	_jdns_rr_data_copy(r, c);
	return c;
}

void jdns_rr_delete(jdns_rr_t *r)
{
	if(!r)
		return;
	if(r->owner)
		jdns_free(r->owner);
	_jdns_rr_data_reset(r);
	jdns_free(r);
}

void jdns_rr_set_owner(jdns_rr_t *r, const unsigned char *name)
{
	if(r->owner)
		jdns_free(r->owner);
	r->owner = _ustrdup(name);
}

void jdns_rr_set_record(jdns_rr_t *r, int type, const unsigned char *rdata, int rdlength)
{
	_jdns_rr_data_reset(r);
	r->type = type;
	r->rdlength = rdlength;
	r->rdata = jdns_copy_array(rdata, rdlength);
}

void jdns_rr_set_A(jdns_rr_t *r, const jdns_address_t *address)
{
	_jdns_rr_data_reset(r);
	r->type = JDNS_RTYPE_A;
	r->haveKnown = 1;
	r->data.address = jdns_address_copy(address);
}

void jdns_rr_set_AAAA(jdns_rr_t *r, const jdns_address_t *address)
{
	_jdns_rr_data_reset(r);
	r->type = JDNS_RTYPE_AAAA;
	r->haveKnown = 1;
	r->data.address = jdns_address_copy(address);
}

void jdns_rr_set_MX(jdns_rr_t *r, const unsigned char *name, int priority)
{
	jdns_server_t *s = jdns_server_new();
	jdns_server_set_name(s, name);
	s->priority = priority;

	_jdns_rr_data_reset(r);
	r->type = JDNS_RTYPE_MX;
	r->haveKnown = 1;
	r->data.server = s;
}

void jdns_rr_set_SRV(jdns_rr_t *r, const unsigned char *name, int port, int priority, int weight)
{
	jdns_server_t *s = jdns_server_new();
	jdns_server_set_name(s, name);
	s->port = port;
	s->priority = priority;
	s->weight = weight;

	_jdns_rr_data_reset(r);
	r->type = JDNS_RTYPE_SRV;
	r->haveKnown = 1;
	r->data.server = s;
}

void jdns_rr_set_CNAME(jdns_rr_t *r, const unsigned char *name)
{
	_jdns_rr_data_reset(r);
	r->type = JDNS_RTYPE_CNAME;
	r->haveKnown = 1;
	r->data.name = _ustrdup(name);
}

void jdns_rr_set_PTR(jdns_rr_t *r, const unsigned char *name)
{
	_jdns_rr_data_reset(r);
	r->type = JDNS_RTYPE_PTR;
	r->haveKnown = 1;
	r->data.name = _ustrdup(name);
}

void jdns_rr_set_TXT(jdns_rr_t *r, const jdns_stringlist_t *texts)
{
	_jdns_rr_data_reset(r);
	r->type = JDNS_RTYPE_TXT;
	r->haveKnown = 1;
	r->data.texts = jdns_stringlist_copy(texts);
}

void jdns_rr_set_HINFO(jdns_rr_t *r, const jdns_string_t *cpu, const jdns_string_t *os)
{
	_jdns_rr_data_reset(r);
	r->type = JDNS_RTYPE_HINFO;
	r->haveKnown = 1;
	r->data.hinfo.cpu = jdns_string_copy(cpu);
	r->data.hinfo.os = jdns_string_copy(os);
}

void jdns_rr_set_NS(jdns_rr_t *r, const unsigned char *name)
{
	_jdns_rr_data_reset(r);
	r->type = JDNS_RTYPE_NS;
	r->haveKnown = 1;
	r->data.name = _ustrdup(name);
}

int jdns_rr_verify(const jdns_rr_t *r)
{
	if(r->type == -1)
		return 0;

	if(!jdns_packet_name_isvalid(r->owner, _ustrlen(r->owner)))
		return 0;

	switch(r->type)
	{
		case JDNS_RTYPE_MX:
		case JDNS_RTYPE_SRV:
		{
			// consider it valid if we don't have a known to check
			if(!r->haveKnown)
				return 1;
			if(!jdns_packet_name_isvalid(r->data.server->name, _ustrlen(r->data.server->name)))
				return 0;
			break;
		}
		case JDNS_RTYPE_CNAME:
		case JDNS_RTYPE_PTR:
		case JDNS_RTYPE_NS:
		{
			if(!r->haveKnown)
				return 1;
			if(!jdns_packet_name_isvalid(r->data.name, _ustrlen(r->data.name)))
				return 0;
			break;
		}
		case JDNS_RTYPE_TXT:
		{
			int n;
			if(!r->haveKnown)
				return 1;
			for(n = 0; n < r->data.texts->count; ++n)
			{
				if(r->data.texts->item[n]->size > 255)
					return 0;
			}
			break;
		}
		case JDNS_RTYPE_HINFO:
		{
			if(!r->haveKnown)
				return 1;
			if(r->data.hinfo.cpu->size > 255)
				return 0;
			if(r->data.hinfo.os->size > 255)
				return 0;
			break;
		}
	}

	return 1;
}

static jdns_string_t *read_name_at_end(const jdns_packet_resource_t *pr, const jdns_packet_t *ref, int _at)
{
	jdns_string_t *name;
	int at;
	at = _at;
	if(!jdns_packet_resource_read_name(pr, ref, &at, &name))
		return 0;
	if(at != pr->rdlength)
	{
		jdns_string_delete(name);
		return 0;
	}
	return name;
}

static jdns_string_t *read_text_string(const jdns_packet_resource_t *pr, int *_at)
{
	jdns_string_t *out;
	int at, len;
	at = *_at;
	if(at + 1 > pr->rdlength)
		return 0;
	len = pr->rdata[at++];
	if(at + len > pr->rdlength)
		return 0;
	out = jdns_string_new();
	jdns_string_set(out, pr->rdata + at, len);
	at += len;
	*_at = at;
	return out;
}

// if the type is known, then it must be parsed properly
// if the type is unknown, then that's ok
// rdata is always copied, known or not
jdns_rr_t *jdns_rr_from_resource(const jdns_packet_resource_t *pr, const jdns_packet_t *ref)
{
	jdns_rr_t *rr = 0;

	if(pr->qtype == JDNS_RTYPE_ANY)
		return 0;

	switch(pr->qtype)
	{
		case JDNS_RTYPE_A:
		{
			jdns_address_t *addr;
			unsigned long int ip;
			if(pr->rdlength != 4)
				break;
			memcpy(&ip, pr->rdata, 4);
			ip = ntohl(ip);
			addr = jdns_address_new();
			jdns_address_set_ipv4(addr, ip);
			rr = jdns_rr_new();
			jdns_rr_set_A(rr, addr);
			jdns_address_delete(addr);
			break;
		}
		case JDNS_RTYPE_AAAA:
		{
			jdns_address_t *addr;
			if(pr->rdlength != 16)
				break;
			addr = jdns_address_new();
			jdns_address_set_ipv6(addr, pr->rdata);
			rr = jdns_rr_new();
			jdns_rr_set_AAAA(rr, addr);
			jdns_address_delete(addr);
			break;
		}
		case JDNS_RTYPE_MX:
		{
			unsigned short priority;
			jdns_string_t *name;
			if(pr->rdlength < 2)
				break;
			memcpy(&priority, pr->rdata, 2);
			priority = ntohs(priority);
			name = read_name_at_end(pr, ref, 2);
			if(!name)
				break;
			rr = jdns_rr_new();
			jdns_rr_set_MX(rr, name->data, priority);
			jdns_string_delete(name);
			break;
		}
		case JDNS_RTYPE_SRV:
		{
			unsigned short priority, weight, port;
			jdns_string_t *name;
			if(pr->rdlength < 6)
				break;
			memcpy(&priority, pr->rdata, 2);
			priority = ntohs(priority);
			memcpy(&weight, pr->rdata + 2, 2);
			weight = ntohs(weight);
			memcpy(&port, pr->rdata + 4, 2);
			port = ntohs(port);
			name = read_name_at_end(pr, ref, 6);
			if(!name)
				break;
			rr = jdns_rr_new();
			jdns_rr_set_SRV(rr, name->data, port, priority, weight);
			jdns_string_delete(name);
			break;
		}
		case JDNS_RTYPE_CNAME:
		{
			jdns_string_t *name;
			name = read_name_at_end(pr, ref, 0);
			if(!name)
				break;
			rr = jdns_rr_new();
			jdns_rr_set_CNAME(rr, name->data);
			jdns_string_delete(name);
			break;
		}
		case JDNS_RTYPE_PTR:
		{
			jdns_string_t *name;
			name = read_name_at_end(pr, ref, 0);
			if(!name)
				break;
			rr = jdns_rr_new();
			jdns_rr_set_PTR(rr, name->data);
			jdns_string_delete(name);
			break;
		}
		case JDNS_RTYPE_TXT:
		{
			jdns_stringlist_t *texts;
			jdns_string_t *str;
			int at, error;
			texts = jdns_stringlist_new();
			at = 0;
			error = 0;
			while(at < pr->rdlength)
			{
				str = read_text_string(pr, &at);
				if(!str)
				{
					error = 1;
					break;
				}
				jdns_stringlist_append(texts, str);
				jdns_string_delete(str);
			}
			if(error)
			{
				jdns_stringlist_delete(texts);
				break;
			}
			rr = jdns_rr_new();
			jdns_rr_set_TXT(rr, texts);
			jdns_stringlist_delete(texts);
			break;
		}
		case JDNS_RTYPE_HINFO:
		{
			jdns_string_t *cpu, *os;
			int at;
			at = 0;
			cpu = read_text_string(pr, &at);
			if(!cpu)
				break;
			os = read_text_string(pr, &at);
			if(!os)
			{
				jdns_string_delete(cpu);
				break;
			}
			if(at != pr->rdlength)
			{
				jdns_string_delete(cpu);
				jdns_string_delete(os);
				break;
			}
			rr = jdns_rr_new();
			jdns_rr_set_HINFO(rr, cpu, os);
			jdns_string_delete(cpu);
			jdns_string_delete(os);
			break;
		}
		case JDNS_RTYPE_NS:
		{
			jdns_string_t *name;
			name = read_name_at_end(pr, ref, 0);
			if(!name)
				break;
			rr = jdns_rr_new();
			jdns_rr_set_NS(rr, name->data);
			jdns_string_delete(name);
			break;
		}
		default:
		{
			rr = jdns_rr_new();
			rr->type = pr->qtype;
			break;
		}
	}

	if(rr)
	{
		rr->qclass = pr->qclass;
		rr->owner = _ustrdup(pr->qname->data);
		rr->ttl = (int)pr->ttl; // pr->ttl is 31-bits, cast is safe
		rr->rdlength = pr->rdlength;
		rr->rdata = jdns_copy_array(pr->rdata, pr->rdlength);
	}

	return rr;
}

//----------------------------------------------------------------------------
// jdns_response
//----------------------------------------------------------------------------
#define ARRAY_DELETE(array, count, dtor) \
	{ \
		if(count > 0) \
		{ \
			int n; \
			for(n = 0; n < count; ++n) \
				dtor(array[n]); \
		} \
		jdns_free(array); \
		array = 0; \
		count = 0; \
	}

#define ARRAY_COPY(type, array_src, count_src, array_dest, count_dest, cctor) \
	{ \
		if(count_src > 0) \
		{ \
			int n; \
			count_dest = count_src; \
			array_dest = (type **)jdns_alloc(sizeof(type *) * count_dest); \
			for(n = 0; n < count_dest; ++n) \
				array_dest[n] = cctor(array_src[n]); \
		} \
	}

#define ARRAY_APPEND(type, array, count, item) \
	{ \
		if(!array) \
			array = (type **)jdns_alloc(sizeof(type *)); \
		else \
			array = (type **)jdns_realloc(array, sizeof(type *) * (count + 1)); \
		array[count] = item; \
		++count; \
	}

jdns_response_t *jdns_response_new()
{
	jdns_response_t *r = alloc_type(jdns_response_t);
	r->answerCount = 0;
	r->answerRecords = 0;
	r->authorityCount = 0;
	r->authorityRecords = 0;
	r->additionalCount = 0;
	r->additionalRecords = 0;
	return r;
}

jdns_response_t *jdns_response_copy(const jdns_response_t *r)
{
	jdns_response_t *c = jdns_response_new();
	ARRAY_COPY(jdns_rr_t, r->answerRecords, r->answerCount, c->answerRecords, c->answerCount, jdns_rr_copy);
	ARRAY_COPY(jdns_rr_t, r->authorityRecords, r->authorityCount, c->authorityRecords, c->authorityCount, jdns_rr_copy);
	ARRAY_COPY(jdns_rr_t, r->additionalRecords, r->additionalCount, c->additionalRecords, c->additionalCount, jdns_rr_copy);
	return c;
}

void jdns_response_delete(jdns_response_t *r)
{
	if(!r)
		return;
	ARRAY_DELETE(r->answerRecords, r->answerCount, jdns_rr_delete);
	ARRAY_DELETE(r->authorityRecords, r->authorityCount, jdns_rr_delete);
	ARRAY_DELETE(r->additionalRecords, r->additionalCount, jdns_rr_delete);
	jdns_free(r);
}

void jdns_response_append_answer(jdns_response_t *r, const jdns_rr_t *rr)
{
	ARRAY_APPEND(jdns_rr_t, r->answerRecords, r->answerCount, jdns_rr_copy(rr));
}

void jdns_response_append_authority(jdns_response_t *r, const jdns_rr_t *rr)
{
	ARRAY_APPEND(jdns_rr_t, r->authorityRecords, r->authorityCount, jdns_rr_copy(rr));
}

void jdns_response_append_additional(jdns_response_t *r, const jdns_rr_t *rr)
{
	ARRAY_APPEND(jdns_rr_t, r->additionalRecords, r->additionalCount, jdns_rr_copy(rr));
}

void jdns_response_remove_extra(jdns_response_t *r)
{
	ARRAY_DELETE(r->authorityRecords, r->authorityCount, jdns_rr_delete);
	ARRAY_DELETE(r->additionalRecords, r->additionalCount, jdns_rr_delete);
}

void jdns_response_remove_answer(jdns_response_t *r, int pos)
{
	jdns_rr_t *rr = r->answerRecords[pos];
	jdns_rr_delete(rr);

	// free the position
	if(r->answerCount > 1)
	{
		memmove(r->answerRecords + pos, r->answerRecords + pos + 1, (r->answerCount - pos - 1) * sizeof(void *));
		--r->answerCount;
	}
	else
	{
		jdns_free(r->answerRecords);
		r->answerRecords = 0;
		r->answerCount = 0;
	}
}
