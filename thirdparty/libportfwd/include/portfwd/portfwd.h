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

#ifndef LIBPORTFWD_PORTFWD_H
#define LIBPORTFWD_PORTFWD_H

#include "portfwddllmacro.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdio>
#include <list>
#include <iostream>

struct UPNPUrls;
struct IGDdatas;

class PORTFWDDLLEXPORT Portfwd
{
    public:
        Portfwd();
        ~Portfwd();

        /// timeout: milliseconds to wait for a router to respond
        /// 2000 is typically enough.
        bool init( unsigned int timeout );
        void get_status();

        bool add( unsigned short port, unsigned short internal_port );
        bool remove( unsigned short port );

        void addBlockedDevice( const std::string& ip );

        const std::string& external_ip() const { return m_externalip; }
        const std::string& lan_ip() const { return m_lanip; }
        unsigned int max_upstream_bps() const { return m_upbps; }
        unsigned int max_downstream_bps() const { return m_downbps; }

    protected:
        struct UPNPUrls* m_urls;
        struct IGDdatas* m_data;

        std::list<std::string> m_blockedips;
        std::string m_lanip, m_externalip;
        unsigned int m_upbps, m_downbps;
};

#endif