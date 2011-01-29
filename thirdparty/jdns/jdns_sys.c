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

/*
this code probes the system for dns settings.  blah.

q3dns strategies
----------------

windows:

domain name, name server, "search list" found in windows registry here:

  HKEY_LOCAL_MACHINE
  System\CurrentControlSet\Services\Tcpip\Parameters  <-- win nt+
  System\CurrentControlSet\Services\VxD\MSTCP  <-- win 98

  for domain, try DhcpDomain else Domain
  for name servers, try DhcpNameServer, else NameServer
  for search list, try SearchList

iphlpapi.dll : GetNetworkParams(PFIXED_INFO, PULONG);

  info->DomainName
  info->DnsServerList (if not null, use it, and loop through ->Next until
    null)
  no search list

first try getnetworkparams.  if that fails, try the nt regkey then the 98
  regkey.  it seems that search list can only come from the registry, so
  maybe we need to grab that entry even if getnetworkparams works.

in the case of the registry, the nameserver and searchlist entries are
  delimited by spaces on win nt and commas on win 98.  probably a good
  idea to simplify white space first (chop away space at start and end,
  reduce all sections of spaces to one char).  also, lowercase the search
  list.

qt doesn't read the hosts file on windows.  this might be a good idea, but
  probably not worth it.

unix:

read /etc/resolv.conf manually:
  for each line, split at spaces
  if the first item is "nameserver", then there should be an IP address
    following it.  note: may contain mixed ipv4 and ipv6 addresses
  if the first item is "search", all other items are added to the domain
    list
  if the first item is "domain", then the next item should be added to the
    domain list.
  do case-insensitive matching for the item types
  for search/domain, the items are in the 8-bit system locale

info can also be fetched using system calls.  we use the res_* stuff here.
  first we should detect for a "modern res api".  this is available from
  glibc 2.3 and onward.  use the following scheme to check for it:

#if defined(__GLIBC__) && ((__GLIBC__ > 2) || ((__GLIBC__ == 2)
  && (__GLIBC_MINOR__ >= 3)))
  // modern res api
#endif

on mac we should look up res_init in the system library.  see:
  qt_mac_resolve_sys(RTLD_NEXT, "res_init"); for a hint.
otherwise we can just use res_init() straight.

under a modern res api, we do:
  struct __res_state res;
  res_ninit(&res);
otherwise, we simply call res_init().  for the modern api, we use the "res"
  struct that we made.  otherwise, we use the global "_res" struct.

read the res struct to obtain the name servers, search list, and domain.
  lowercase the search list and domain.

qt tries the file, and if that fails it tries the syscalls.  we may want to
  do the syscalls first, or even just do both all the time.

read /etc/hosts manually:
  for each line
    if there is a '#' character in the line, remove it and everything to
      the right
    simplify white space
    convert to lowercase
  split the line at spaces
  first item is the ip address
  all remaining items are hostnames

  note: these hosts could also be used for reverse-dns too
  note2: Windows has a hosts file as well (like C:\WINDOWS\hosts)
*/

#include "jdns_p.h"

#ifdef JDNS_OS_WIN
# include <windows.h>
#endif

#ifdef JDNS_OS_UNIX
# include <netinet/in.h>
# include <arpa/nameser.h>
# include <resolv.h>
# include <dlfcn.h>
#endif

#define string_indexOf jdns_string_indexOf
#define string_split jdns_string_split

static int char_isspace(unsigned char c)
{
	if(c == ' ' || c == '\t' || c == '\n' || c == '\r')
		return 1;
	return 0;
}

static unsigned char *string_getnextword(unsigned char *in, int size, int pos, int *newpos)
{
	int n;
	int at;
	int len;
	unsigned char *out;

	at = pos;

	// skip any space at the start
	while(at < size && char_isspace(in[at]))
		++at;

	// all space?  no word then
	if(at >= size)
		return 0;

	// skip until a space or end
	n = at;
	while(n < size && !char_isspace(in[n]))
		++n;
	len = n - at;

	// allocate length + zero byte
	out = (unsigned char *)jdns_alloc(len + 1);
	if(!out)
		return 0;
	memcpy(out, in + at, len);
	out[len] = 0;
	*newpos = at + len;
	return out;
}

static jdns_string_t *string_simplify(const jdns_string_t *in)
{
	int n;
	int pos;
	int total;
	unsigned char *out;
	int outlen;
	jdns_string_t *outstr;
	jdns_stringlist_t *wordlist;

	// gather words and total of lengths
	pos = 0;
	total = 0;
	wordlist = jdns_stringlist_new();
	while(1)
	{
		jdns_string_t *word;
		unsigned char *str = string_getnextword(in->data, in->size, pos, &pos);
		if(!str)
			break;
		word = jdns_string_new();
		jdns_string_set_cstr(word, (char *)str);
		jdns_free(str);
		jdns_stringlist_append(wordlist, word);
		total += word->size;
		jdns_string_delete(word);
	}

	if(total == 0)
	{
		jdns_stringlist_delete(wordlist);

		outstr = jdns_string_new();
		jdns_string_set_cstr(outstr, "");
		return outstr;
	}

	// we need to allocate space for total lengths and wordcount-1 spaces
	outlen = total + (wordlist->count - 1);
	out = (unsigned char *)jdns_alloc(outlen);

	// lay out the words
	pos = 0;
	for(n = 0; n < wordlist->count; ++n)
	{
		unsigned char *data = wordlist->item[n]->data;
		int size = wordlist->item[n]->size;
		memcpy(out + pos, data, size);
		pos += size;

		// if this is not the last word, append a space
		if(n + 1 < wordlist->count)
			out[pos++] = ' ';
	}
	jdns_stringlist_delete(wordlist);

	outstr = jdns_string_new();
	jdns_string_set(outstr, out, outlen);
	jdns_free(out);
	return outstr;
}

static jdns_string_t *string_tolower(const jdns_string_t *in)
{
	int n;
	jdns_string_t *out = jdns_string_copy(in);
	for(n = 0; n < out->size; ++n)
		out->data[n] = tolower(out->data[n]);
	return out;
}

static jdns_string_t *file_nextline(FILE *f)
{
	int at, size;
	unsigned char *buf;
	jdns_string_t *str;

	size = 1023;
	buf = (unsigned char *)jdns_alloc(size);
	at = 0;
	while(1)
	{
		unsigned char c = fgetc(f);
		if(feof(f))
		{
			if(at > 0)
			{
				// if we read at least one char, take it as a
				//   line
				break;
			}
			else
			{
				jdns_free(buf);
				return 0;
			}
		}
		if(c == '\n')
			break;
		if(c == '\r')
			continue;
		if(at < 1023)
			buf[at++] = c;
	}

	str = jdns_string_new();
	jdns_string_set(str, buf, at);
	jdns_free(buf);
	return str;
}

static jdns_dnshostlist_t *read_hosts_file(const char *path)
{
	jdns_dnshostlist_t *out;
	FILE *f;
	jdns_string_t *line, *simp;
	jdns_stringlist_t *parts;
	jdns_address_t *addr;
	int n;

	out = jdns_dnshostlist_new();

	f = jdns_fopen(path, "r");
	if(!f)
		return out;
	while(1)
	{
		line = file_nextline(f);
		if(!line)
			break;

		// truncate at comment
		n = string_indexOf(line, '#', 0);
		if(n != -1)
		{
			line->size = n;
			line->data[n] = 0;
		}

		simp = string_simplify(line);
		jdns_string_delete(line);

		parts = string_split(simp, ' ');
		jdns_string_delete(simp);

		if(parts->count < 2)
		{
			jdns_stringlist_delete(parts);
			continue;
		}

		addr = jdns_address_new();
		if(!jdns_address_set_cstr(addr, (const char *)parts->item[0]->data))
		{
			jdns_address_delete(addr);
			jdns_stringlist_delete(parts);
			continue;
		}

		for(n = 1; n < parts->count; ++n)
		{
			jdns_dnshost_t *h = jdns_dnshost_new();
			h->name = jdns_string_copy(parts->item[n]);
			h->address = jdns_address_copy(addr);
			jdns_dnshostlist_append(out, h);
			jdns_dnshost_delete(h);
		}

		jdns_address_delete(addr);
		jdns_stringlist_delete(parts);
	}
	fclose(f);
	return out;
}

static void apply_hosts_file(jdns_dnsparams_t *a, const char *path)
{
	int n;
	jdns_dnshostlist_t *list;

	list = read_hosts_file(path);
	for(n = 0; n < list->count; ++n)
		jdns_dnshostlist_append(a->hosts, list->item[n]);
	jdns_dnshostlist_delete(list);
}

static int dnsparams_have_domain(const jdns_dnsparams_t *a, const jdns_string_t *domain)
{
	int n;
	for(n = 0; n < a->domains->count; ++n)
	{
		jdns_string_t *str = a->domains->item[n];
		if(strcmp((const char *)str->data, (const char *)domain->data) == 0)
			return 1;
	}
	return 0;
}

#ifdef JDNS_OS_WIN

// from Microsoft IPTypes.h
#ifndef IP_TYPES_INCLUDED
#define MAX_HOSTNAME_LEN    128
#define MAX_DOMAIN_NAME_LEN 128
#define MAX_SCOPE_ID_LEN    256
typedef struct {
    char String[4 * 4];
} IP_ADDRESS_STRING, *PIP_ADDRESS_STRING, IP_MASK_STRING, *PIP_MASK_STRING;
typedef struct _IP_ADDR_STRING {
    struct _IP_ADDR_STRING* Next;
    IP_ADDRESS_STRING IpAddress;
    IP_MASK_STRING IpMask;
    DWORD Context;
} IP_ADDR_STRING, *PIP_ADDR_STRING;
typedef struct {
    char HostName[MAX_HOSTNAME_LEN + 4] ;
    char DomainName[MAX_DOMAIN_NAME_LEN + 4];
    PIP_ADDR_STRING CurrentDnsServer;
    IP_ADDR_STRING DnsServerList;
    UINT NodeType;
    char ScopeId[MAX_SCOPE_ID_LEN + 4];
    UINT EnableRouting;
    UINT EnableProxy;
    UINT EnableDns;
} FIXED_INFO, *PFIXED_INFO;
#endif

typedef DWORD (WINAPI *GetNetworkParamsFunc)(PFIXED_INFO, PULONG);

static jdns_string_t *reg_readString(HKEY hk, const char *subkey)
{
	char *buf;
	DWORD bufsize;
	int ret;
	jdns_string_t *str = 0;

	bufsize = 1024;
	buf = (char *)jdns_alloc((int)bufsize);
	if(!buf)
		return 0;
	ret = RegQueryValueExA(hk, subkey, 0, 0, (LPBYTE)buf, &bufsize);
	if(ret == ERROR_MORE_DATA)
	{
		buf = (char *)jdns_realloc(buf, bufsize);
		if(!buf)
		{
			jdns_free(buf);
			return 0;
		}
		ret = RegQueryValueExA(hk, subkey, 0, 0, (LPBYTE)buf, &bufsize);
	}
	if(ret == ERROR_SUCCESS)
	{
		str = jdns_string_new();
		jdns_string_set_cstr(str, (char *)buf);
	}
	jdns_free(buf);
	return str;
}

static jdns_dnsparams_t *dnsparams_get_winreg()
{
	int n;
	jdns_dnsparams_t *params;
	HKEY key;
	int ret;
	char sep;
	jdns_string_t *str_domain, *str_nameserver, *str_searchlist;
	jdns_stringlist_t *list_nameserver, *list_searchlist;

	sep = ' ';
	ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
		"System\\CurrentControlSet\\Services\\Tcpip\\Parameters",
		0, KEY_READ, &key);
	if(ret != ERROR_SUCCESS)
	{
		sep = ',';
		ret = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
			"System\\CurrentControlSet\\Services\\VxD\\MSTCP",
			0, KEY_READ, &key);
		if(ret != ERROR_SUCCESS)
			return 0;
	}

	str_domain = reg_readString(key, "DhcpDomain");
	if(!str_domain)
		str_domain = reg_readString(key, "Domain");
	str_nameserver = reg_readString(key, "DhcpNameServer");
	if(!str_nameserver)
		str_nameserver = reg_readString(key, "NameServer");
	str_searchlist = reg_readString(key, "SearchList");

	RegCloseKey(key);

	list_nameserver = 0;
	if(str_nameserver)
	{
		list_nameserver = string_split(str_nameserver, sep);
		jdns_string_delete(str_nameserver);
	}
	list_searchlist = 0;
	if(str_searchlist)
	{
		// lowercase the string
		jdns_string_t *p = string_tolower(str_searchlist);
		jdns_string_delete(str_searchlist);
		str_searchlist = p;

		list_searchlist = string_split(str_searchlist, sep);
		jdns_string_delete(str_searchlist);
	}

	params = jdns_dnsparams_new();
	if(list_nameserver)
	{
		// qt seems to do a strange thing here by running each name
		//   server address through the q3dns setLabel function, and
		//   then pulls the result as a list of addresses.  i have
		//   no idea why they do this, or how one IP address would
		//   turn into anything else, let alone several addresses.
		// so, uh, we're not going to do that.
		for(n = 0; n < list_nameserver->count; ++n)
		{
			jdns_address_t *addr = jdns_address_new();
			if(jdns_address_set_cstr(addr, (char *)list_nameserver->item[n]->data))
				jdns_dnsparams_append_nameserver(params, addr, JDNS_UNICAST_PORT);
			jdns_address_delete(addr);
		}
		jdns_stringlist_delete(list_nameserver);
	}
	if(str_domain)
	{
		if(str_domain->size > 0)
			jdns_dnsparams_append_domain(params, str_domain);
		jdns_string_delete(str_domain);
	}
	if(list_searchlist)
	{
		for(n = 0; n < list_searchlist->count; ++n)
		{
			if(list_searchlist->item[n]->size > 0)
				jdns_dnsparams_append_domain(params, list_searchlist->item[n]);
		}
		jdns_stringlist_delete(list_searchlist);
	}

	return params;
}

static jdns_dnsparams_t *dnsparams_get_winsys()
{
	jdns_dnsparams_t *params;
	GetNetworkParamsFunc myGetNetworkParams;
	DWORD ret;
	HINSTANCE lib;
	jdns_address_t *addr;
	jdns_string_t *str;
	IP_ADDR_STRING *ipstr;

	lib = LoadLibraryA("iphlpapi");
	if(!lib)
		return 0;

	params = 0;
	myGetNetworkParams = (GetNetworkParamsFunc)GetProcAddress(lib, "GetNetworkParams");
	if(myGetNetworkParams)
	{
		ULONG bufsize = 0;
		ret = myGetNetworkParams(0, &bufsize);
		if(ret == ERROR_BUFFER_OVERFLOW)
		{
			FIXED_INFO *info = (FIXED_INFO *)jdns_alloc((int)bufsize);
			ret = myGetNetworkParams(info, &bufsize);
			if(ret == ERROR_SUCCESS)
			{
				params = jdns_dnsparams_new();
				ipstr = &info->DnsServerList;
				while(ipstr)
				{
					addr = jdns_address_new();
					if(jdns_address_set_cstr(addr, (char *)ipstr->IpAddress.String))
						jdns_dnsparams_append_nameserver(params, addr, JDNS_UNICAST_PORT);
					jdns_address_delete(addr);
					ipstr = ipstr->Next;
				}
				str = jdns_string_new();
				jdns_string_set_cstr(str, info->DomainName);
				if(str->size > 0)
					jdns_dnsparams_append_domain(params, str);
				jdns_string_delete(str);
			}
			jdns_free(info);
		}
	}
	FreeLibrary(lib);
	return params;
}

static void apply_hosts_var_filepath(jdns_dnsparams_t *a, const char *envvar, const char *path)
{
	jdns_string_t *e;
	char *str;
	int elen, plen;

	e = jdns_getenv(envvar);
	if(!e)
		return;
	elen = strlen((char *)e->data);
	plen = strlen(path);
	str = (char *)jdns_alloc(elen + plen + 1);
	memcpy(str, e->data, elen);
	jdns_string_delete(e);

	jdns_strcpy(str + elen, path);
	apply_hosts_file(a, str);
	jdns_free(str);
}

static void apply_win_hosts_file(jdns_dnsparams_t *a)
{
	// windows 64-bit
	apply_hosts_var_filepath(a, "SystemRoot", "\\SysWOW64\\drivers\\etc\\hosts");

	// winnt+
	apply_hosts_var_filepath(a, "SystemRoot", "\\system32\\drivers\\etc\\hosts");

	// win9x
	apply_hosts_var_filepath(a, "WINDIR", "\\hosts");
}

static jdns_dnsparams_t *dnsparams_get_win()
{
	int n;
	jdns_dnsparams_t *sys_params, *reg_params;

	reg_params = dnsparams_get_winreg();
	sys_params = dnsparams_get_winsys();

	// no sys params?  take the reg params then
	if(!sys_params)
	{
		apply_win_hosts_file(reg_params);
		return reg_params;
	}

	// sys params don't have a search list, so merge the domains from
	//   the registry if possible
	if(reg_params)
	{
		for(n = 0; n < reg_params->domains->count; ++n)
		{
			jdns_string_t *reg_str = reg_params->domains->item[n];

			// don't add dups
			if(!dnsparams_have_domain(sys_params, reg_str))
				jdns_dnsparams_append_domain(sys_params, reg_str);
		}
		jdns_dnsparams_delete(reg_params);
	}
	apply_win_hosts_file(sys_params);
	return sys_params;
}

#endif

#ifdef JDNS_OS_UNIX

static jdns_dnsparams_t *dnsparams_get_unixfiles()
{
	FILE *f;
	int n;
	jdns_dnsparams_t *params;
	jdns_string_t *line, *simp;
	jdns_stringlist_t *parts;

	params = jdns_dnsparams_new();

	f = jdns_fopen("/etc/resolv.conf", "r");
	if(!f)
		return params;
	while(1)
	{
		line = file_nextline(f);
		if(!line)
			break;

		// truncate at comment
		n = string_indexOf(line, '#', 0);
		if(n != -1)
		{
			line->size = n;
			line->data[n] = 0;
		}

		simp = string_simplify(line);
		jdns_string_delete(line);

		parts = string_split(simp, ' ');
		jdns_string_delete(simp);

		if(parts->count < 2)
		{
			jdns_stringlist_delete(parts);
			continue;
		}

		simp = string_tolower(parts->item[0]);
		if(strcmp((char *)simp->data, "nameserver") == 0)
		{
			jdns_address_t *addr = jdns_address_new();
			jdns_address_set_cstr(addr, (const char *)parts->item[1]->data);
			jdns_dnsparams_append_nameserver(params, addr, JDNS_UNICAST_PORT);
			jdns_address_delete(addr);
		}
		else if(strcmp((char *)simp->data, "search") == 0)
		{
			for(n = 1; n < parts->count; ++n)
			{
				jdns_dnsparams_append_domain(params, parts->item[n]);
			}
		}
		else if(strcmp((char *)simp->data, "domain") == 0)
		{
			jdns_dnsparams_append_domain(params, parts->item[1]);
		}
		jdns_string_delete(simp);

		jdns_stringlist_delete(parts);
	}
	fclose(f);
	return params;
}

#if defined(__GLIBC__) && ((__GLIBC__ > 2) || ((__GLIBC__ == 2) && (__GLIBC_MINOR__ >= 3)))
# define JDNS_MODERN_RES_API
#endif

#ifndef JDNS_MODERN_RES_API
typedef int (*res_init_func)();
static int my_res_init()
{
#ifdef JDNS_OS_MAC
	res_init_func mac_res_init;

	// look up res_init in the system library (qt does this, not sure why)
	mac_res_init = (res_init_func)dlsym(RTLD_NEXT, "res_init");
	if(!mac_res_init)
		return -1;
	return mac_res_init();
#else
	return res_init();
#endif
}
#endif

// on some platforms, __res_state_ext exists as a struct but it is not
//   a define, so the #ifdef doesn't work.  as a workaround, we'll explicitly
//   specify the platforms that have __res_state_ext
//#ifdef __res_state_ext
#if defined(JDNS_OS_MAC) || defined(JDNS_OS_FREEBSD) || \
    defined(JDNS_OS_NETBSD) || defined (JDNS_OS_SOLARIS)
# define USE_EXTEXT
#endif

static jdns_dnsparams_t *dnsparams_get_unixsys()
{
	int n;
	jdns_dnsparams_t *params;

#ifdef JDNS_MODERN_RES_API
	struct __res_state res;
	memset(&res, 0, sizeof(struct __res_state));
	n = res_ninit(&res);
#define RESVAR res
#else
	n = my_res_init();
#define RESVAR _res
#endif

	params = jdns_dnsparams_new();

	// error initializing?
	if(n == -1)
		return params;

	// nameservers - ipv6
	for(n = 0; n < MAXNS && n < RESVAR._u._ext.nscount; ++n)
	{
		jdns_address_t *addr;
		struct sockaddr_in6 *sa6;

#ifdef USE_EXTEXT
		sa6 = ((struct sockaddr_in6 *)RESVAR._u._ext.ext) + n;
#else
		sa6 = RESVAR._u._ext.nsaddrs[n];
#endif

		if(sa6 == NULL)
			continue;
		addr = jdns_address_new();
		jdns_address_set_ipv6(addr, sa6->sin6_addr.s6_addr);
		jdns_dnsparams_append_nameserver(params, addr, JDNS_UNICAST_PORT);
		jdns_address_delete(addr);
	}

	// nameservers - ipv4
	for(n = 0; n < MAXNS && n < RESVAR.nscount; ++n)
	{
		jdns_address_t *addr = jdns_address_new();
		jdns_address_set_ipv4(addr, ntohl(RESVAR.nsaddr_list[n].sin_addr.s_addr));
		jdns_dnsparams_append_nameserver(params, addr, JDNS_UNICAST_PORT);
		jdns_address_delete(addr);
	}

	// domain name
	if(strlen(RESVAR.defdname) > 0)
	{
		jdns_string_t *str;
		jdns_string_t *p;
		str = jdns_string_new();
		jdns_string_set_cstr(str, RESVAR.defdname);
		p = string_tolower(str);
		jdns_string_delete(str);
		str = p;
		jdns_dnsparams_append_domain(params, str);
		jdns_string_delete(str);
	}

	// search list
#ifdef MAXDFLSRCH
	for(n = 0; n < MAXDFLSRCH && RESVAR.dnsrch[n]; ++n)
	{
		if(strlen(RESVAR.dnsrch[n]) > 0)
		{
			jdns_string_t *str;
			jdns_string_t *p;
			str = jdns_string_new();
			jdns_string_set_cstr(str, RESVAR.dnsrch[n]);
			p = string_tolower(str);
			jdns_string_delete(str);
			str = p;

			// don't add dups
			if(!dnsparams_have_domain(params, str))
				jdns_dnsparams_append_domain(params, str);

			jdns_string_delete(str);
		}
	}
#endif

	return params;
}

static jdns_dnsparams_t *dnsparams_get_unix()
{
	jdns_dnsparams_t *params;

	// prefer system calls over files
	params = dnsparams_get_unixsys();
	if(params->nameservers->count == 0)
	{
		jdns_dnsparams_delete(params);
		params = dnsparams_get_unixfiles();
	}

	apply_hosts_file(params, "/etc/hosts");

	return params;
}

#endif

jdns_dnsparams_t *jdns_system_dnsparams()
{
#ifdef JDNS_OS_WIN
	return dnsparams_get_win();
#else
	return dnsparams_get_unix();
#endif
}
