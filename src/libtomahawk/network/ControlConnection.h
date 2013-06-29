/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
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

/*
    One ControlConnection always remains open to each peer.

    They arrange connections/reverse connections, inform us
    when the peer goes offline, and own+setup DBSyncConnections.

*/
#ifndef CONTROLCONNECTION_H
#define CONTROLCONNECTION_H

#include "Connection.h"
#include "DllMacro.h"
#include "Typedefs.h"

class ControlConnectionPrivate;
class DBSyncConnection;
class Servent;

class DLLEXPORT ControlConnection : public Connection
{
Q_OBJECT

public:
    ControlConnection( Servent* parent );
    ~ControlConnection();
    Connection* clone();

    DBSyncConnection* dbSyncConnection();

    Tomahawk::source_ptr source() const;

    /**
     * Tell this ControlConnection that is no longer controlling the source and should not do any actions on it.
     */
    void unbindFromSource();

    void addPeerInfo( const Tomahawk::peerinfo_ptr& peerInfo );
    void removePeerInfo( const Tomahawk::peerinfo_ptr& peerInfo );
    void setShutdownOnEmptyPeerInfos( bool shutdownOnEmptyPeerInfos );
    const QSet< Tomahawk::peerinfo_ptr > peerInfos() const;

protected:
    virtual void setup();

protected slots:
    virtual void handleMsg( msg_ptr msg );
    virtual void authCheckTimeout();

private slots:
    void dbSyncConnFinished( QObject* c );
    void registerSource();
    void onPingTimer();

private:
    Q_DECLARE_PRIVATE( ControlConnection )
    ControlConnectionPrivate* d_ptr;

    void setupDbSyncConnection( bool ondemand = false );
};

#endif // CONTROLCONNECTION_H
