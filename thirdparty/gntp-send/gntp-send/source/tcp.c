#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include "tcp.h"

int growl_tcp_parse_hostname( const char *const server , int default_port , struct sockaddr_in *const sockaddr  );

void growl_tcp_write( int sock , const char *const format , ... ) 
{
	int length;
	char *output;

	va_list ap;

	va_start( ap , format );
	length = vsnprintf( NULL , 0 , format , ap );
	va_end(ap);

	va_start(ap,format);
	output = (char*)malloc(length+1);
	vsnprintf( output , length+1 , format , ap );
	va_end(ap);

	send( sock , output , length , 0 );
	send( sock , "\r\n" , 2 , 0 );

	free(output);
}

char *growl_tcp_read(int sock) {
	const int growsize = 80;
	char c = 0;
	char* line = (char*) malloc(growsize);
	int len = growsize, pos = 0;
	while (line) {
		if (recv(sock, &c, 1, 0) <= 0) break;
		if (c == '\r') continue;
		if (c == '\n') break;
		line[pos++] = c;
		if (pos >= len) {
			len += growsize;
			line = (char*) realloc(line, len);
		}
	}
	line[pos] = 0;
	return line;
}

int growl_tcp_open(const char* server) {
	int sock = -1;
	struct sockaddr_in serv_addr;

	if( growl_tcp_parse_hostname( server , 23053 , &serv_addr ) == -1 )
	{
		return -1;
	}

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("create socket");
		return -1;
	}

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("connect");
		return -1;
	}

	return sock;
}

void growl_tcp_close(int sock) {
#ifdef _WIN32
	if (sock > 0) closesocket(sock);
#else
	if (sock > 0) close(sock);
#endif
}

int growl_tcp_parse_hostname( const char *const server , int default_port , struct sockaddr_in *const sockaddr )
{
	char *hostname = strdup(server);
	char *port = strchr( hostname, ':' );
	struct hostent* host_ent;
	if( port != NULL )
	{
		*port = '\0';
		port++;
		default_port = atoi(port);
	}
	
	host_ent = gethostbyname(hostname);
	if( host_ent == NULL )
	{
		perror("gethostbyname");
		free(hostname);
		return -1;
	}
	
	memset( sockaddr , 0 , sizeof(sockaddr) );
	sockaddr->sin_family = AF_INET;
	memcpy( &sockaddr->sin_addr , host_ent->h_addr , host_ent->h_length );
	sockaddr->sin_port = htons(default_port);
	 
	free(hostname);
	return 0;
}

int growl_tcp_datagram( const char *server , const unsigned char *data , const int data_length )
{
	struct sockaddr_in serv_addr;
	int sock = 0;

	if( growl_tcp_parse_hostname( server , 9887 , &serv_addr ) == -1 )
	{
		return -1;
	}
	
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if( sock < 0 )
	{
		return -1;
	}
	
	if( sendto(sock, (char*)data , data_length , 0 , (struct sockaddr*)&serv_addr , sizeof(serv_addr) ) > 0 )
	{
		return 0;
	}
	else
	{
		return 1;
	}
}
