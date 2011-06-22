#include <growl++.hpp>
#include <growl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


Growl::Growl(const Growl_Protocol _protocol, const char *const _password, const char *const _application, const char **_notifications, const int _notifications_count)
{
	server = strdup("localhost");
	password = strdup(_password);
	protocol = _protocol;
	application = strdup(_application);
	Register(_notifications, _notifications_count);
}


Growl::Growl(const Growl_Protocol _protocol, const char *const _server, const char *const _password, const char *const _application, const char **_notifications, const int _notifications_count )
{
	server = strdup(_server);
	password = strdup(_password);
	protocol = _protocol;
	application = strdup(_application);
	Register(_notifications, _notifications_count);
}


void Growl::Register(const char **const notifications, const int notifications_count , const char *const icon )
{
	if( protocol == GROWL_TCP )
	{
		growl_tcp_register( server , application , notifications , notifications_count , password , icon );
	}
	else
	{
		growl_udp_register( server , application , notifications , notifications_count , password );
	}
}


Growl::~Growl()
{
	if(server != NULL)	
	{
		free(server);
	}
	if(password != NULL)
	{
		free(password);
	}
	if(application == NULL)
	{
		free(application);
	}
}


void Growl::Notify(const char *const notification, const char *const title, const char* const message)
{
	Growl::Notify(notification, title, message, NULL, NULL);
}


void Growl::Notify(const char *const notification, const char *const title, const char* const message, const char *const url, const char *const icon)
{
	if( protocol == GROWL_TCP )
        {
                growl_tcp_notify( server , application , notification , title , message , password , url , icon );
        }
        else
        {
                growl_udp_notify( server , application , notification , title , message , password );
        }
}
