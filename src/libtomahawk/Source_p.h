/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef SOURCE_P_H
#define SOURCE_P_H

#include "Source.h"

#include <QTimer>

namespace Tomahawk
{

class SourcePrivate
{
public:
    SourcePrivate( Source* q, int _id, const QString& _nodeid )
        : q_ptr ( q )
        , isLocal( false )
        , online( false )
        , nodeId( _nodeid )
        , id( _id )
        , updateIndexWhenSynced( false )
        , state( UNKNOWN )
        , avatar( 0 )
        , avatarLoaded( false )
        , cc( 0 )
        , commandCount( 0 )
    {
    }
    Source* q_ptr;
    Q_DECLARE_PUBLIC ( Source )

private:
    QList< QSharedPointer<Collection> > collections;
    QVariantMap stats;

    bool isLocal;
    bool online;
    QString nodeId;
    QString friendlyname;
    QString dbFriendlyName;
    int id;
    bool scrubFriendlyName;
    bool updateIndexWhenSynced;

    Tomahawk::query_ptr currentTrack;
    QString textStatus;
    DBSyncConnectionState state;
    QTimer currentTrackTimer;

    QPixmap* avatar;
    bool avatarLoaded;

    QPointer<ControlConnection> cc;
    QList< Tomahawk::dbcmd_ptr > cmds;
    int commandCount;
    QString lastCmdGuid;
    QMutex setControlConnectionMutex;
    QMutex mutex;

    Tomahawk::playlistinterface_ptr playlistInterface;

    mutable QMutex cmdMutex;
};

}

#endif // SOURCE_P_H
