/*
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "portfwd/portfwd.h"

#include "miniwget.h"
#include "miniupnpc.h"
#include "upnpcommands.h"

#ifdef WIN32
#include <winsock2.h>
#include "../include/portfwd/portfwd.h"
#endif

Portfwd::Portfwd()
 : urls(0), data(0)
{
}

Portfwd::~Portfwd()
{
    if(urls) free(urls);
    if(data) free(data);
}

bool
Portfwd::init(unsigned int timeout)
{
#ifdef WIN32
    WSADATA wsaData;
    int nResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if(nResult != NO_ERROR)
    {
        fprintf(stderr, "WSAStartup() failed.\n");
        return -1;
    }
#endif
   struct UPNPDev * devlist;
   struct UPNPDev * dev;
   char * descXML;
   int descXMLsize = 0;
   printf("Portfwd::init()\n");
   urls = (UPNPUrls*)malloc(sizeof(struct UPNPUrls));
   data = (IGDdatas*)malloc(sizeof(struct IGDdatas));
   memset(urls, 0, sizeof(struct UPNPUrls));
   memset(data, 0, sizeof(struct IGDdatas));
   devlist = upnpDiscover(timeout, NULL, NULL, 0);
   if (devlist)
   {
       dev = devlist;
       while (dev)
       {
           if (strstr (dev->st, "InternetGatewayDevice"))
               break;
           dev = dev->pNext;
       }
       if (!dev)
           dev = devlist; /* defaulting to first device */

       printf("UPnP device :\n"
              " desc: %s\n st: %s\n",
              dev->descURL, dev->st);

       descXML = (char*)miniwget(dev->descURL, &descXMLsize);
       if (descXML)
       {
           parserootdesc (descXML, descXMLsize, data);
           free (descXML); descXML = 0;
           GetUPNPUrls (urls, data, dev->descURL);
       }
       else
       {
           printf("couldn't get the UPnP device description XML (descXML is null)");
           freeUPNPDevlist(devlist);
           return false;
       }
       // get lan IP:
       char lanaddr[16];
       int idg_was_found = UPNP_GetValidIGD(devlist, urls, data, (char*)&lanaddr, 16);
       printf("UPNP_GetValidIGD returned %d", idg_was_found);
       if (!idg_was_found)
       {
           printf("NO IGD was found (function UPNP_GetValidIGD())");
           freeUPNPDevlist(devlist);
           return false;
       }
       m_lanip = std::string(lanaddr);

       freeUPNPDevlist(devlist);
       get_status();
       return true;
   }
   return false;
}

void
Portfwd::get_status()
{
//    Q_ASSERT(urls->controlURL_CIF != NULL);
    // get connection speed
    UPNP_GetLinkLayerMaxBitRates(
        urls->controlURL_CIF, data->CIF.servicetype, &m_downbps, &m_upbps);

    // get external IP adress
    char ip[16];
    if( 0 != UPNP_GetExternalIPAddress( urls->controlURL,
                                        data->CIF.servicetype,
                                        (char*)&ip ) )
    {
        m_externalip = ""; //failed
    }else{
        m_externalip = std::string(ip);
    }
}

bool
Portfwd::add( unsigned short port, unsigned short internal_port )
{
   char port_str[16], port_str_internal[16];
   int r;
   printf("Portfwd::add (%s, %d)\n", m_lanip.c_str(), port);
   if(urls->controlURL[0] == '\0')
   {
       printf("Portfwd - the init was not done !\n");
       return false;
   }
   sprintf(port_str, "%d", port);
   sprintf(port_str_internal, "%d", internal_port);

   r = UPNP_AddPortMapping(urls->controlURL, data->first.servicetype,
                           port_str, port_str_internal, m_lanip.c_str(), "tomahawk", "TCP", NULL);
   if(r!=0)
   {
    printf("AddPortMapping(%s, %s, %s) failed, code %d\n", port_str, port_str, m_lanip.c_str(), r);
    return false;
   }
   return true;
}

bool
Portfwd::remove( unsigned short port )
{
   char port_str[16];
   printf("Portfwd::remove(%d)\n", port);
   if(urls->controlURL[0] == '\0')
   {
       printf("Portfwd - the init was not done !\n");
       return false;
   }
   sprintf(port_str, "%d", port);
   int r = UPNP_DeletePortMapping(urls->controlURL, data->first.servicetype, port_str, "TCP", NULL);
   return r == 0;
}

