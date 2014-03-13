/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef SERVENT_P_H
#define SERVENT_P_H

#include "Servent.h"

#include <QMutex>

#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <qjson/qobjecthelper.h>

#include <boost/function.hpp>

class ServentPrivate : public QObject
{
Q_OBJECT

public:
    ServentPrivate( Servent* q )
        : q_ptr ( q )
        , port( 0 )
        , externalPort( 0 )
        , ready( false )
    {
    }
    Servent* q_ptr;
    Q_DECLARE_PUBLIC ( Servent )

private:
    QMap< QString, QPointer< Connection > > offers;
    QMap< QString, QPair< Tomahawk::peerinfo_ptr, QString > > lazyoffers;
    QStringList connectedNodes;
    QJson::Parser parser;

    /**
     * canonical list of authed peers
     */
    QMutex controlconnectionsMutex;
    QList< ControlConnection* > controlconnections;

    /**
     * The external port used by all address except those obtained via UPnP or the static configuration option
     */
    int port;

    /**
     * Either the static set or the UPnP set external port
     */
    int externalPort;

    /**
     * All available external IPs
     */
    QList<QHostAddress> externalAddresses;

    /**
     * Either the static set or the UPnP set external host
     */
    QString externalHostname;

    bool ready;
    bool noAuth;

    // currently active file transfers:
    QList< StreamConnection* > scsessions;
    QMutex ftsession_mut;
    // username -> nodeid -> PeerInfos
    QMap<QString, QMap<QString, QSet<Tomahawk::peerinfo_ptr> > > queuedForACLResult;

    QPointer< PortFwdThread > portfwd;
};

#endif // SERVENT_P_H
