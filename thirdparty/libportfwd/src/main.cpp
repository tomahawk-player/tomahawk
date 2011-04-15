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

int main(int argc, char** argv)
{
    if(argc!=2)
    {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);
    Portfwd pf;
    if(!pf.init(2000))
    {
        printf("Portfwd.init() failed.\n");
        return 2;
    }
    printf("External IP: %s\n", pf.external_ip().c_str());
    printf("LAN IP: %s\n", pf.lan_ip().c_str());
    printf("Max upstream: %d bps, max downstream: %d bps\n",
           pf.max_upstream_bps(), pf.max_downstream_bps() );
           
    printf("%s\n", ((pf.add( port ))?"Added":"Failed to add") );

    printf("Any key to exit...\n");
    char foo;
    scanf("%c",&foo);

    printf("%s\n",  ((pf.remove( port ))?"Removed.":"Failed to remove") );
    return 0;
}

