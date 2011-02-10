/*
 * Copyright (C) 2006  Justin Karneges
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

#include "jdns_packet.h"

#include "jdns_p.h"

// maximum length of a sublabel
#define MAX_SUBLABEL_LENGTH  63

// maximum length of a label, including final terminating zero (root sublabel)
//   according to RFC 2181, the maximum length is 255, not counting the root
//   sublabel.  so, with the root sublabel, that means a max length of 256.
#define MAX_LABEL_LENGTH     256

// jer's endian functions
static unsigned short int net2short(const unsigned char **bufp)
{
	unsigned short int i;
	i = **bufp;
	i <<= 8;
	i |= *(*bufp + 1);
	*bufp += 2;
	return i;
}

static unsigned long int net2long(const unsigned char **bufp)
{
	unsigned long int l;
	l = **bufp;
	l <<= 8;
	l |= *(*bufp + 1);
	l <<= 8;
	l |= *(*bufp + 2);
	l <<= 8;
	l |= *(*bufp + 3);
	*bufp += 4;
	return l;
}

static void short2net(unsigned short int i, unsigned char **bufp)
{
	*(*bufp + 1) = (unsigned char)i;
	i >>= 8;
	**bufp = (unsigned char)i;
	*bufp += 2;    
}

static void long2net(unsigned long int l, unsigned char **bufp)
{
	*(*bufp + 3) = (unsigned char)l;
	l >>= 8;
	*(*bufp + 2) = (unsigned char)l;
	l >>= 8;
	*(*bufp + 1) = (unsigned char)l;
	l >>= 8;
	**bufp = (unsigned char)l;
	*bufp += 4;
}

// label stuff
typedef struct jdns_packet_label
{
	JDNS_OBJECT
	int offset;
	jdns_string_t *value;
} jdns_packet_label_t;

static void jdns_packet_label_delete(jdns_packet_label_t *a);
static jdns_packet_label_t *jdns_packet_label_copy(const jdns_packet_label_t *a);

static jdns_packet_label_t *jdns_packet_label_new()
{
	jdns_packet_label_t *a = JDNS_OBJECT_NEW(jdns_packet_label);
	a->offset = 0;
	a->value = 0;
	return a;
}

jdns_packet_label_t *jdns_packet_label_copy(const jdns_packet_label_t *a)
{
	jdns_packet_label_t *c = jdns_packet_label_new();
	c->offset = a->offset;
	if(a->value)
		c->value = jdns_string_copy(a->value);
	return c;
}

void jdns_packet_label_delete(jdns_packet_label_t *a)
{
	if(!a)
		return;
	jdns_string_delete(a->value);
	jdns_object_free(a);
}

// gets an offset for decompression.  does range and hop count checking also
static int getoffset(const unsigned char *str, int refsize, int *hopsleft)
{
	unsigned short int x;
	if(*hopsleft <= 0)
		return -1;
	--(*hopsleft);
	x = str[0] & 0x3f;
	x <<= 8;
	x |= str[1];
	// stay in bounds
	if(x >= refsize)
		return -1;
	return x;
}

static int readlabel(const unsigned char *in, int insize, const unsigned char *ref, int refsize, int *_at, jdns_string_t **name)
{
	int at;
	// string format is one character smaller than dns format.  e.g.:
	//   dns:    [7] affinix [3] com [0] = 13 bytes
	//   string: "affinix.com."          = 12 bytes
	// only exception is '.' itself, but that won't influence the max.
	unsigned char out[MAX_LABEL_LENGTH - 1];
	int out_size;
	const unsigned char *label, *last;
	int hopped_yet;
	int hopsleft;
	int label_size;

	at = *_at;

	// stay in range
	if(at < 0 || at >= insize)
		return 0;

	out_size = 0;
	label = in + at;
	hopped_yet = 0;
	last = in + insize;
	while(1)
	{
		// need a byte
		if(label + 1 > last)
			goto error;

		// we make this a while loop instead of an 'if', in case
		//   there's a pointer to a pointer.  as a precaution,
		//   we will hop no more than 8 times
		hopsleft = 8;
		while(*label & 0xc0)
		{
			int offset;

			// need the next byte, too
			if(label + 2 > last)
				goto error;

			offset = getoffset(label, refsize, &hopsleft);
			if(offset == -1)
				goto error;

			label = ref + offset;
			if(!hopped_yet)
			{
				at += 2;
				hopped_yet = 1;
				last = ref + refsize;
			}

			// need a byte
			if(label + 1 > last)
				goto error;
		}

		label_size = *label & 0x3f;

		// null label?  then we're done
		if(label_size == 0)
		{
			if(!hopped_yet)
				++at;
			break;
		}

		// enough source bytes? (length byte + length)
		if(label + label_size + 1 > last)
			goto error;

		// enough dest bytes? (length + dot)
		if(out_size + label_size + 1 > MAX_LABEL_LENGTH - 1)
			goto error;

		memcpy(out + out_size, label + 1, label_size);
		out_size += label_size;
		out[out_size] = '.';
		++out_size;

		if(!hopped_yet)
			at += label_size + 1;

		label += label_size + 1;
	}

	*_at = at;
	*name = jdns_string_new();
	jdns_string_set(*name, out, out_size);
	return 1;

error:
	return 0;
}

// this function compares labels in label format:
//   [length] [value ...] [length] [value ...] [0]
static int matchlabel(const unsigned char *a, int asize, const unsigned char *b, int bsize, const unsigned char *ref, int refsize, int ahopsleft, int bhopsleft)
{
	int n, alen, blen, offset;

	// same pointer?
	if(a == b)
		return 1;

	if(asize < 1 || bsize < 1)
		return 0;

	// always ensure we get called without a pointer
	if(*a & 0xc0)
	{
		if(asize < 2)
			return 0;
		offset = getoffset(a, refsize, &ahopsleft);
		if(offset == -1)
			return 0;
		return matchlabel(ref + offset, refsize - offset, b, bsize, ref, refsize, ahopsleft, bhopsleft);
	}
	if(*b & 0xc0)
	{
		if(bsize < 2)
			return 0;
		offset = getoffset(b, refsize, &bhopsleft);
		if(offset == -1)
			return 0;
		return matchlabel(a, asize, ref + offset, refsize - offset, ref, refsize, ahopsleft, bhopsleft);
	}

	alen = *a & 0x3f;
	blen = *b & 0x3f;

	// must be same length
	if(alen != blen)
		return 0;

	// done?
	if(alen == 0)
		return 1;

	// length byte + length + first byte of next label
	if(asize < alen + 2)
		return 0;
	if(bsize < blen + 2)
		return 0;

	// compare the value
	for(n = 1; n < alen + 1; ++n)
	{
		if(a[n] != b[n])
			return 0;
	}

	// try next labels
	n = alen + 1;
	return matchlabel(a + n, asize - n, b + n, bsize - n, ref, refsize, ahopsleft, bhopsleft);
}

int jdns_packet_name_isvalid(const unsigned char *name, int size)
{
	int n, at, len;

	// at least one byte, no larger than MAX_LABEL_LENGTH - 1 (one byte is
	//   gained when converting to a label)
	if(size < 1 || size > (MAX_LABEL_LENGTH - 1))
		return 0;

	// last byte must be a dot
	if(name[size - 1] != '.')
		return 0;

	// first byte can't be a dot if there are characters after
	if(size > 1 && name[0] == '.')
		return 0;

	// each sublabel must be between 1 and MAX_SUBLABEL_LENGTH in length
	at = 0;
	while(1)
	{
		// search for dot or end
		for(n = at; n < size; ++n)
		{
			if(name[n] == '.')
				break;
		}
		// length of last one is always zero
		if(n >= size)
			break;

		len = n - at;
		if(len < 1 || len > MAX_SUBLABEL_LENGTH)
			return 0;
		at = n + 1; // skip over the dot
	}

	return 1;
}

// this function assumes label is pointing to a MAX_LABEL_LENGTH byte buffer
static int name_to_label(const jdns_string_t *name, unsigned char *label)
{
	int n, i, at, len;

	if(!jdns_packet_name_isvalid(name->data, name->size))
		return -1;

	if(name->size == 1)
	{
		label[0] = 0;
		return 1;
	}

	at = 0;
	i = 0;
	while(1)
	{
		// search for dot or end
		for(n = at; n < name->size; ++n)
		{
			if(name->data[n] == '.')
				break;
		}
		len = n - at;
		if(i + (len + 1) > MAX_LABEL_LENGTH) // length byte + length
			return 0;

		label[i++] = len;
		memcpy(label + i, name->data + at, len);
		i += len;

		if(n >= name->size) // end?
			break;
		at = n + 1; // skip over the dot
	}

	return i;
}

// lookup list is made of jdns_packet_labels
static int writelabel(const jdns_string_t *name, int at, int left, unsigned char **bufp, jdns_list_t *lookup)
{
	unsigned char label[MAX_LABEL_LENGTH];
	int n, i, len;
	unsigned char *l;
	unsigned char *ref;
	int refsize;

	len = name_to_label(name, label);
	if(len == -1)
		return 0;

	ref = *bufp - at;
	refsize = at + left;
	for(n = 0; label[n]; n += label[n] + 1)
	{
		for(i = 0; i < lookup->count; ++i)
		{
			jdns_packet_label_t *pl = (jdns_packet_label_t *)lookup->item[i];

			if(matchlabel(label + n, len - n, pl->value->data, pl->value->size, ref, refsize, 8, 8))
			{
				// set up a pointer right here, overwriting
				//   the length byte and the first content
				//   byte of this section within 'label'.
				//   this is safe, because the length value
				//   will always be greater than zero,
				//   ensuring we have two bytes available to
				//   use.
				l = label + n;
				short2net((unsigned short int)pl->offset, &l);
				label[n] |= 0xc0;
				len = n + 2; // cut things short
				break;
			}
		}
		if(label[n] & 0xc0) // double loop, so break again
			break;
	}

	if(left < len)
		return 0;

	// copy into buffer, point there now
	memcpy(*bufp, label, len);
	l = *bufp;
	*bufp += len;

	// for each new label, store its location for future compression
	for(n = 0; l[n]; n += l[n] + 1)
	{
		jdns_string_t *str;
		jdns_packet_label_t *pl;
		if(l[n] & 0xc0)
			break;

		pl = jdns_packet_label_new();
		str = jdns_string_new();
		jdns_string_set(str, l + n, len - n);
		pl->offset = l + n - ref;
		pl->value = str;
		jdns_list_insert(lookup, pl, -1);
	}

	return 1;
}

//----------------------------------------------------------------------------
// jdns_packet_write
//----------------------------------------------------------------------------
#define JDNS_PACKET_WRITE_RAW  0
#define JDNS_PACKET_WRITE_NAME 1

struct jdns_packet_write
{
	JDNS_OBJECT
	int type;
	jdns_string_t *value;
};

void jdns_packet_write_delete(jdns_packet_write_t *a);
jdns_packet_write_t *jdns_packet_write_copy(const jdns_packet_write_t *a);

jdns_packet_write_t *jdns_packet_write_new()
{
	jdns_packet_write_t *a = JDNS_OBJECT_NEW(jdns_packet_write);
	a->type = 0;
	a->value = 0;
	return a;
}

jdns_packet_write_t *jdns_packet_write_copy(const jdns_packet_write_t *a)
{
	jdns_packet_write_t *c = jdns_packet_write_new();
	c->type = a->type;
	if(a->value)
		c->value = jdns_string_copy(a->value);
	return c;
}

void jdns_packet_write_delete(jdns_packet_write_t *a)
{
	if(!a)
		return;
	jdns_string_delete(a->value);
	jdns_object_free(a);
}

//----------------------------------------------------------------------------
// jdns_packet_question
//----------------------------------------------------------------------------
jdns_packet_question_t *jdns_packet_question_new()
{
	jdns_packet_question_t *a = JDNS_OBJECT_NEW(jdns_packet_question);
	a->qname = 0;
	a->qtype = 0;
	a->qclass = 0;
	return a;
}

jdns_packet_question_t *jdns_packet_question_copy(const jdns_packet_question_t *a)
{
	jdns_packet_question_t *c = jdns_packet_question_new();
	if(a->qname)
		c->qname = jdns_string_copy(a->qname);
	c->qtype = a->qtype;
	c->qclass = a->qclass;
	return c;
}

void jdns_packet_question_delete(jdns_packet_question_t *a)
{
	if(!a)
		return;
	jdns_string_delete(a->qname);
	jdns_object_free(a);
}

//----------------------------------------------------------------------------
// jdns_packet_resource
//----------------------------------------------------------------------------
jdns_packet_resource_t *jdns_packet_resource_new()
{
	jdns_packet_resource_t *a = JDNS_OBJECT_NEW(jdns_packet_resource);
	a->qname = 0;
	a->qtype = 0;
	a->qclass = 0;
	a->ttl = 0;
	a->rdlength = 0;
	a->rdata = 0;

	a->writelog = jdns_list_new();
	a->writelog->valueList = 1;
	return a;
}

jdns_packet_resource_t *jdns_packet_resource_copy(const jdns_packet_resource_t *a)
{
	jdns_packet_resource_t *c = jdns_packet_resource_new();
	if(a->qname)
		c->qname = jdns_string_copy(a->qname);
	c->qtype = a->qtype;
	c->qclass = a->qclass;
	c->ttl = a->ttl;
	c->rdlength = a->rdlength;
	c->rdata = jdns_copy_array(a->rdata, a->rdlength);

	jdns_list_delete(c->writelog);
	c->writelog = jdns_list_copy(a->writelog);
	return c;
}

void jdns_packet_resource_delete(jdns_packet_resource_t *a)
{
	if(!a)
		return;
	jdns_string_delete(a->qname);
	if(a->rdata)
		jdns_free(a->rdata);
	jdns_list_delete(a->writelog);
	jdns_object_free(a);
}

void jdns_packet_resource_add_bytes(jdns_packet_resource_t *a, const unsigned char *data, int size)
{
	jdns_packet_write_t *write = jdns_packet_write_new();
	write->type = JDNS_PACKET_WRITE_RAW;
	write->value = jdns_string_new();
	jdns_string_set(write->value, data, size);
	jdns_list_insert_value(a->writelog, write, -1);
	jdns_packet_write_delete(write);
}

void jdns_packet_resource_add_name(jdns_packet_resource_t *a, const jdns_string_t *name)
{
	jdns_packet_write_t *write = jdns_packet_write_new();
	write->type = JDNS_PACKET_WRITE_NAME;
	write->value = jdns_string_copy(name);
	jdns_list_insert_value(a->writelog, write, -1);
	jdns_packet_write_delete(write);
}

int jdns_packet_resource_read_name(const jdns_packet_resource_t *a, const jdns_packet_t *p, int *at, jdns_string_t **name)
{
	return readlabel(a->rdata, a->rdlength, p->raw_data, p->raw_size, at, name);
}

//----------------------------------------------------------------------------
// jdns_packet
//----------------------------------------------------------------------------

// note: both process_qsection and process_rrsection modify the 'dest' list,
//   even if later items cause an error.  this turns out to be convenient
//   for handling truncated dns packets

static int process_qsection(jdns_list_t *dest, int count, const unsigned char *data, int size, const unsigned char **bufp)
{
	int n;
	int offset, at;
	jdns_string_t *name = 0;
	const unsigned char *buf;

	buf = *bufp;
	for(n = 0; n < count; ++n)
	{
		jdns_packet_question_t *q;

		offset = buf - data;
		at = 0;

		if(!readlabel(data + offset, size - offset, data, size, &at, &name))
			goto error;

		offset += at;

		// need 4 more bytes
		if(size - offset < 4)
			goto error;

		buf = data + offset;

		q = jdns_packet_question_new();
		q->qname = name;
		name = 0;
		q->qtype = net2short(&buf);
		q->qclass = net2short(&buf);

		jdns_list_insert_value(dest, q, -1);
		jdns_packet_question_delete(q);
	}

	*bufp = buf;
	return 1;

error:
	jdns_string_delete(name);
	return 0;
}

static int process_rrsection(jdns_list_t *dest, int count, const unsigned char *data, int size, const unsigned char **bufp)
{
	int n;
	int offset, at;
	jdns_string_t *name = 0;
	const unsigned char *buf;

	buf = *bufp;
	for(n = 0; n < count; ++n)
	{
		jdns_packet_resource_t *r;

		offset = buf - data;
		at = 0;

		if(!readlabel(data + offset, size - offset, data, size, &at, &name))
			goto error;

		offset += at;

		// need 10 more bytes
		if(offset + 10 > size)
			goto error;

		buf = data + offset;

		r = jdns_packet_resource_new();
		r->qname = name;
		name = 0;
		r->qtype = net2short(&buf);
		r->qclass = net2short(&buf);
		r->ttl = net2long(&buf);

		// per RFC 2181, ttl is supposed to be a 31 bit number.  if
		//   the top bit of the 32 bit field is 1, then entire ttl is
		//   to be considered 0.
		if(r->ttl & 0x80000000)
			r->ttl = 0;

		r->rdlength = net2short(&buf);

		offset = buf - data;

		// make sure we have enough for the rdata
		if(size - offset < r->rdlength)
		{
			jdns_packet_resource_delete(r);
			goto error;
		}

		r->rdata = jdns_copy_array(buf, r->rdlength);
		buf += r->rdlength;

		jdns_list_insert_value(dest, r, -1);
		jdns_packet_resource_delete(r);
	}

	*bufp = buf;
	return 1;

error:
	jdns_string_delete(name);
	return 0;
}

static int append_qsection(const jdns_list_t *src, int at, int left, unsigned char **bufp, jdns_list_t *lookup)
{
	unsigned char *buf, *start, *last;
	int n;

	buf = *bufp;
	start = buf - at;
	last = buf + left;
	for(n = 0; n < src->count; ++n)
	{
		jdns_packet_question_t *q = (jdns_packet_question_t *)src->item[n];

		if(!writelabel(q->qname, buf - start, last - buf, &buf, lookup))
			goto error;

		if(buf + 4 > last)
			goto error;

		short2net(q->qtype, &buf);
		short2net(q->qclass, &buf);
	}

	*bufp = buf;
	return 1;

error:
	return 0;
}

static int append_rrsection(const jdns_list_t *src, int at, int left, unsigned char **bufp, jdns_list_t *lookup)
{
	unsigned char *buf, *start, *last, *rdlengthp;
	int n, i, rdlength;

	buf = *bufp;
	start = buf - at;
	last = buf + left;
	for(n = 0; n < src->count; ++n)
	{
		jdns_packet_resource_t *r = (jdns_packet_resource_t *)src->item[n];

		if(!writelabel(r->qname, buf - start, last - buf, &buf, lookup))
			goto error;

		if(buf + 10 > last)
			goto error;

		short2net(r->qtype, &buf);
		short2net(r->qclass, &buf);
		long2net(r->ttl, &buf);

		// skip over rdlength
		rdlengthp = buf;
		buf += 2;

		// play write log
		rdlength = 0;
		for(i = 0; i < r->writelog->count; ++i)
		{
			jdns_packet_write_t *write = (jdns_packet_write_t *)r->writelog->item[i];
			if(write->type == JDNS_PACKET_WRITE_RAW)
			{
				if(buf + write->value->size > last)
					goto error;

				memcpy(buf, write->value->data, write->value->size);
				buf += write->value->size;
			}
			else // JDNS_PACKET_WRITE_NAME
			{
				if(!writelabel(write->value, buf - start, last - buf, &buf, lookup))
					goto error;
			}
		}

		i = buf - rdlengthp; // should be rdata size + 2
		short2net((unsigned short int)(i - 2), &rdlengthp);
	}

	*bufp = buf;
	return 1;

error:
	return 0;
}

jdns_packet_t *jdns_packet_new()
{
	jdns_packet_t *a = JDNS_OBJECT_NEW(jdns_packet);
	a->id = 0;
	a->opts.qr = 0;
	a->opts.opcode = 0;
	a->opts.aa = 0;
	a->opts.tc = 0;
	a->opts.rd = 0;
	a->opts.ra = 0;
	a->opts.z = 0;
	a->opts.rcode = 0;

	a->questions = jdns_list_new();
	a->answerRecords = jdns_list_new();
	a->authorityRecords = jdns_list_new();
	a->additionalRecords = jdns_list_new();

	a->questions->valueList = 1;
	a->answerRecords->valueList = 1;
	a->authorityRecords->valueList = 1;
	a->additionalRecords->valueList = 1;

	a->fully_parsed = 0;

	a->raw_size = 0;
	a->raw_data = 0;
	return a;
}

jdns_packet_t *jdns_packet_copy(const jdns_packet_t *a)
{
	jdns_packet_t *c = jdns_packet_new();
	c->id = a->id;
	c->opts.qr = a->opts.qr;
	c->opts.opcode = a->opts.opcode;
	c->opts.aa = a->opts.aa;
	c->opts.tc = a->opts.tc;
	c->opts.rd = a->opts.rd;
	c->opts.ra = a->opts.ra;
	c->opts.z = a->opts.z;
	c->opts.rcode = a->opts.rcode;

	jdns_list_delete(c->questions);
	jdns_list_delete(c->answerRecords);
	jdns_list_delete(c->authorityRecords);
	jdns_list_delete(c->additionalRecords);
	c->questions = jdns_list_copy(a->questions);
	c->answerRecords = jdns_list_copy(a->answerRecords);
	c->authorityRecords = jdns_list_copy(a->authorityRecords);
	c->additionalRecords = jdns_list_copy(a->additionalRecords);

	c->fully_parsed = a->fully_parsed;

	c->raw_size = a->raw_size;
	c->raw_data = jdns_copy_array(a->raw_data, a->raw_size);

	return c;
}

void jdns_packet_delete(jdns_packet_t *a)
{
	if(!a)
		return;
	jdns_list_delete(a->questions);
	jdns_list_delete(a->answerRecords);
	jdns_list_delete(a->authorityRecords);
	jdns_list_delete(a->additionalRecords);
	if(a->raw_data)
		jdns_free(a->raw_data);
	jdns_object_free(a);
}

int jdns_packet_import(jdns_packet_t **a, const unsigned char *data, int size)
{
	jdns_packet_t *tmp = 0;
	const unsigned char *buf;

	// need at least some data
	if(!data || size == 0)
		return 0;

	// header (id + options + item counts) is 12 bytes
	if(size < 12)
		goto error;

	tmp = jdns_packet_new();
	buf = data;

	// id
	tmp->id = net2short(&buf);

	// options
	if(buf[0] & 0x80)                        // qr is bit 7
		tmp->opts.qr = 1;
	tmp->opts.opcode = (buf[0] & 0x78) >> 3; // opcode is bits 6,5,4,3
	if(buf[0] & 0x04)                        // aa is bit 2
		tmp->opts.aa = 1;
	if(buf[0] & 0x02)                        // tc is bit 1
		tmp->opts.tc = 1;
	if(buf[0] & 0x01)                        // rd is bit 0
		tmp->opts.rd = 1;
	if(buf[1] & 0x80)                        // ra is bit 7 (second byte)
		tmp->opts.ra = 1;
	tmp->opts.z = (buf[1] & 0x70) >> 4;      // z is bits 6,5,4
	tmp->opts.rcode = buf[1] & 0x0f;         // rcode is bits 3,2,1,0
	buf += 2;

	// item counts
	tmp->qdcount = net2short(&buf);
	tmp->ancount = net2short(&buf);
	tmp->nscount = net2short(&buf);
	tmp->arcount = net2short(&buf);

	// if these fail, we don't count them as errors, since the packet
	//   might have been truncated
	if(!process_qsection(tmp->questions, tmp->qdcount, data, size, &buf))
		goto skip;
	if(!process_rrsection(tmp->answerRecords, tmp->ancount, data, size, &buf))
		goto skip;
	if(!process_rrsection(tmp->authorityRecords, tmp->nscount, data, size, &buf))
		goto skip;
	if(!process_rrsection(tmp->additionalRecords, tmp->arcount, data, size, &buf))
		goto skip;

	tmp->fully_parsed = 1;

skip:
	// keep the raw data for reference during rdata parsing
	tmp->raw_size = size;
	tmp->raw_data = jdns_copy_array(data, size);

	*a = tmp;
	return 1;

error:
	jdns_packet_delete(tmp);
	return 0;
}

int jdns_packet_export(jdns_packet_t *a, int maxsize)
{
	unsigned char *block = 0;
	unsigned char *buf, *last;
	unsigned char c;
	int size;
	jdns_list_t *lookup = 0; // to hold jdns_packet_label_t

	// clear out any existing raw data before we begin
	if(a->raw_data)
	{
		jdns_free(a->raw_data);
		a->raw_data = 0;
		a->raw_size = 0;
	}

	// preallocate
	size = maxsize;
	block = (unsigned char *)jdns_alloc(size);
	memset(block, 0, size);

	buf = block;
	last = block + size;

	if(size < 12)
		goto error;

	short2net(a->id, &buf);
	if(a->opts.qr)
		buf[0] |= 0x80;
	c = (unsigned char)a->opts.opcode;
	buf[0] |= c << 3;
	if(a->opts.aa)
		buf[0] |= 0x04;
	if(a->opts.tc)
		buf[0] |= 0x02;
	if(a->opts.rd)
		buf[0] |= 0x01;
	if(a->opts.ra)
		buf[1] |= 0x80;
	c = (unsigned char)a->opts.z;
	buf[1] |= c << 4;
	c = (unsigned char)a->opts.rcode;
	buf[1] |= c;
	buf += 2;
	short2net((unsigned short int)a->questions->count, &buf);
	short2net((unsigned short int)a->answerRecords->count, &buf);
	short2net((unsigned short int)a->authorityRecords->count, &buf);
	short2net((unsigned short int)a->additionalRecords->count, &buf);

	// append sections
	lookup = jdns_list_new();
	lookup->autoDelete = 1;

	if(!append_qsection(a->questions, buf - block, last - buf, &buf, lookup))
		goto error;
	if(!append_rrsection(a->answerRecords, buf - block, last - buf, &buf, lookup))
		goto error;
	if(!append_rrsection(a->authorityRecords, buf - block, last - buf, &buf, lookup))
		goto error;
	if(!append_rrsection(a->additionalRecords, buf - block, last - buf, &buf, lookup))
		goto error;

	// done with all sections
	jdns_list_delete(lookup);

	// condense
	size = buf - block;
	block = (unsigned char *)jdns_realloc(block, size);

	// finalize
	a->qdcount = a->questions->count;
	a->ancount = a->answerRecords->count;
	a->nscount = a->authorityRecords->count;
	a->arcount = a->additionalRecords->count;
	a->raw_data = block;
	a->raw_size = size;

	return 1;

error:
	jdns_list_delete(lookup);
	if(block)
		jdns_free(block);
	return 0;
}
