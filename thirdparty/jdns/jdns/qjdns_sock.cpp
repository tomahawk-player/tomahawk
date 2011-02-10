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

#include "qjdns_sock.h"

#include <QtGlobal>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef Q_OS_WIN
# include <winsock2.h>
# include <ws2tcpip.h>
#endif

#ifdef Q_OS_UNIX
# include <sys/time.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <fcntl.h>
# include <errno.h>
# include <signal.h>
# include <arpa/inet.h>
#endif

#ifndef QT_NO_IPV6
# define HAVE_IPV6
# ifndef s6_addr
#  define IPPROTO_IPV6 41
   struct in6_addr
   {
     union
     {
       unsigned char  _S6_u8[16];
       unsigned short _S6_u16[8];
       unsigned long  _S6_u32[4];
     } _S6_un;
   };
#  define s6_addr _S6_un._S6_u8
# endif
# ifndef IPV6_JOIN_GROUP
#  define IPV6_JOIN_GROUP 12
#  define IPV6_MULTICAST_HOPS 10
   struct ipv6_mreq
   {
     struct in6_addr ipv6mr_multiaddr;
     unsigned int ipv6mr_interface;
   };
# endif
#endif

static int get_last_error()
{
	int x;
#ifdef Q_OS_WIN
	x = WSAGetLastError();
#else
	x = errno;
#endif
	return x;
}

bool qjdns_sock_setMulticast4(int s, unsigned long int addr, int *errorCode)
{
	int ret;
	struct ip_mreq mc;

	memset(&mc, 0, sizeof(mc));
	mc.imr_multiaddr.s_addr = htonl(addr);
	mc.imr_interface.s_addr = INADDR_ANY;

	ret = setsockopt(s, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const char *)&mc, sizeof(mc));
	if(ret != 0)
	{
		if(errorCode)
			*errorCode = get_last_error();
		return false;
	}
	return true;
}

bool qjdns_sock_setMulticast6(int s, unsigned char *addr, int *errorCode)
{
#ifdef HAVE_IPV6
	int ret;
	struct ipv6_mreq mc;

	memset(&mc, 0, sizeof(mc));
	memcpy(mc.ipv6mr_multiaddr.s6_addr, addr, 16);
	mc.ipv6mr_interface = 0;

	ret = setsockopt(s, IPPROTO_IPV6, IPV6_JOIN_GROUP, (const char *)&mc, sizeof(mc));
	if(ret != 0)
	{
		if(errorCode)
			*errorCode = get_last_error();
		return false;
	}
	return true;
#else
	Q_UNUSED(s);
	Q_UNUSED(addr);
	Q_UNUSED(errorCode);
	return false;
#endif
}

bool qjdns_sock_setTTL4(int s, int ttl)
{
	unsigned char cttl;
	int ret, ittl;

	cttl = ttl;
	ittl = ttl;

	// IP_MULTICAST_TTL might take 1 byte or 4, try both
	ret = setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (const char *)&cttl, sizeof(cttl));
	if(ret != 0)
	{
		ret = setsockopt(s, IPPROTO_IP, IP_MULTICAST_TTL, (const char *)&ittl, sizeof(ittl));
		if(ret != 0)
			return false;
	}
	return true;
}

bool qjdns_sock_setTTL6(int s, int ttl)
{
#ifdef HAVE_IPV6
	unsigned char cttl;
	int ret, ittl;

	cttl = ttl;
	ittl = ttl;

	// IPV6_MULTICAST_HOPS might take 1 byte or 4, try both
	ret = setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (const char *)&cttl, sizeof(cttl));
	if(ret != 0)
	{
		ret = setsockopt(s, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, (const char *)&ittl, sizeof(ittl));
		if(ret != 0)
			return false;
	}
	return true;
#else
	Q_UNUSED(s);
	Q_UNUSED(ttl);
	return false;
#endif
}

bool qjdns_sock_setIPv6Only(int s)
{
#if defined(HAVE_IPV6) && defined(IPV6_V6ONLY)
	int x = 1;
	if(setsockopt(s, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&x, sizeof(x)) != 0)
		return false;
	return true;
#else
	Q_UNUSED(s);
	return false;
#endif
}
