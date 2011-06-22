#ifdef _WIN32
#include <winsock2.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "md5.h"
#include "tcp.h"
#include "growl.h"

static char* string_to_utf8_alloc(const char* str) {
#ifdef _WIN32
	unsigned int codepage;
	size_t in_len = strlen(str);
	wchar_t* wcsdata;
	char* mbsdata;
	size_t mbssize, wcssize;

	codepage = GetACP();
	wcssize = MultiByteToWideChar(codepage, 0, str, in_len,  NULL, 0);
	wcsdata = (wchar_t*) malloc((wcssize + 1) * sizeof(wchar_t));
	wcssize = MultiByteToWideChar(codepage, 0, str, in_len, wcsdata, wcssize + 1);
	wcsdata[wcssize] = 0;

	mbssize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) wcsdata, -1, NULL, 0, NULL, NULL);
	mbsdata = (char*) malloc((mbssize + 1));
	mbssize = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR) wcsdata, -1, mbsdata, mbssize, NULL, NULL);
	mbsdata[mbssize] = 0;
	free(wcsdata);
	return mbsdata;
#else
	return strdup(str);
#endif
}

int	opterr = 1;
int	optind = 1;
int	optopt;
char *optarg;

int getopts(int argc, char** argv, char* opts) {
	static int sp = 1;
	register int c;
	register char *cp;

	if(sp == 1) {
		if(optind >= argc ||
				argv[optind][0] != '-' || argv[optind][1] == '\0')
			return(EOF);
		else if(strcmp(argv[optind], "--") == 0) {
			optind++;
			return(EOF);
		}
	}
	optopt = c = argv[optind][sp];
	if(c == ':' || (cp=strchr(opts, c)) == NULL) {
		if(argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		return('?');
	}
	if(*++cp == ':') {
		if(argv[optind][sp+1] != '\0')
			optarg = &argv[optind++][sp+1];
		else if(++optind >= argc) {
			sp = 1;
			return('?');
		} else
			optarg = argv[optind++];
		sp = 1;
	} else {
		if(argv[optind][++sp] == '\0') {
			sp = 1;
			optind++;
		}
		optarg = NULL;
	}
	return(c);
}

int main(int argc, char* argv[]) {
	int c;
	int rc;
	char* server = NULL;
	char* password = NULL;
	char* appname = "gntp-send";
	char* notify = "gntp-send notify";
	char* title = NULL;
	char* message = NULL;
	char* icon = NULL;
	char* url = NULL;
	int tcpsend = 1;

	opterr = 0;
	while ((c = getopts(argc, argv, "a:n:s:p:u") != -1)) {
		switch (optopt) {
		case 'a': appname = optarg; break;
		case 'n': notify = optarg; break;
		case 's': server = optarg; break;
		case 'p': password = optarg; break;
		case 'u': tcpsend = 0; break;
		case '?': break;
		default: argc = 0; break;
		}
		optarg = NULL;
	}

	if ((argc - optind) < 2 || (argc - optind) > 4) {
		fprintf(stderr, "%s: [-u] [-a APPNAME] [-n NOTIFY] [-s SERVER:PORT] [-p PASSWORD] title message [icon] [url]\n", argv[0]);
		exit(1);
	}

	title = string_to_utf8_alloc(argv[optind]);
	message = string_to_utf8_alloc(argv[optind + 1]);
	if ((argc - optind) >= 3) icon = string_to_utf8_alloc(argv[optind + 2]);
	if ((argc - optind) == 4) url = string_to_utf8_alloc(argv[optind + 3]);

	if (!server) server = "127.0.0.1";

	growl_init();	
	if (tcpsend) {
		rc = growl(server,appname,notify,title,message,icon,password,url);
	} else {
		rc = growl_udp(server,appname,notify,title,message,icon,password,url);
	}
	growl_shutdown();

	if (title) free(title);
	if (message) free(message);
	if (icon) free(icon);
	if (url) free(url);

	return rc;
}
