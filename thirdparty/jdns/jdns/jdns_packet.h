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

#ifndef JDNS_PACKET_H
#define JDNS_PACKET_H

#include "jdns.h"

// -- howto --
//
// writing packets:
//   1) call jdns_packet_new()
//   2) populate the jdns_packet_t structure, using the functions
//      as necessary
//   3) call jdns_packet_export() to populate the raw data of the packet
//
// reading packets:
//   1) call jdns_packet_new()
//   2) call jdns_packet_import() with the raw data
//   3) the jdns_packet_t structure is now populated
//
// IMPORTANT: all names must be valid. that is, ending in a dot character

int jdns_packet_name_isvalid(const unsigned char *name, int size); // 0 if not valid

typedef struct jdns_packet_question
{
	JDNS_OBJECT
	jdns_string_t *qname;
	unsigned short int qtype, qclass;
} jdns_packet_question_t;

jdns_packet_question_t *jdns_packet_question_new();
jdns_packet_question_t *jdns_packet_question_copy(const jdns_packet_question_t *a);
void jdns_packet_question_delete(jdns_packet_question_t *a);

typedef struct jdns_packet_write jdns_packet_write_t;
typedef struct jdns_packet jdns_packet_t;

typedef struct jdns_packet_resource
{
	JDNS_OBJECT
	jdns_string_t *qname;
	unsigned short int qtype, qclass;
	unsigned long int ttl; // 31-bit number, top bit always 0
	unsigned short int rdlength;
	unsigned char *rdata;

	// private
	jdns_list_t *writelog; // jdns_packet_write_t
} jdns_packet_resource_t;

jdns_packet_resource_t *jdns_packet_resource_new();
jdns_packet_resource_t *jdns_packet_resource_copy(const jdns_packet_resource_t *a);
void jdns_packet_resource_delete(jdns_packet_resource_t *a);
void jdns_packet_resource_add_bytes(jdns_packet_resource_t *a, const unsigned char *data, int size);
void jdns_packet_resource_add_name(jdns_packet_resource_t *a, const jdns_string_t *name);
int jdns_packet_resource_read_name(const jdns_packet_resource_t *a, const jdns_packet_t *p, int *at, jdns_string_t **name);

struct jdns_packet
{
	JDNS_OBJECT
	unsigned short int id;
	struct
	{
		unsigned short qr, opcode, aa, tc, rd, ra, z, rcode;
	} opts;

	// item counts as specified by the packet.  do not use these
	//   for iteration over the item lists, since they can be wrong
	//   if the packet is truncated.
	int qdcount, ancount, nscount, arcount;

	// value lists
	jdns_list_t *questions;         // jdns_packet_question_t
	jdns_list_t *answerRecords;     // jdns_packet_resource_t
	jdns_list_t *authorityRecords;  // jdns_packet_resource_t
	jdns_list_t *additionalRecords; // jdns_packet_resource_t

	// since dns packets are allowed to be truncated, it is possible
	//   for a packet to not get fully parsed yet still be considered
	//   successfully parsed.  this flag means the packet was fully
	//   parsed also.
	int fully_parsed;

	int raw_size;
	unsigned char *raw_data;
};

jdns_packet_t *jdns_packet_new();
jdns_packet_t *jdns_packet_copy(const jdns_packet_t *a);
void jdns_packet_delete(jdns_packet_t *a);
int jdns_packet_import(jdns_packet_t **a, const unsigned char *data, int size); // 0 on fail
int jdns_packet_export(jdns_packet_t *a, int maxsize); // 0 on fail

#endif
