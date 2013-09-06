/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013     , Uwe L. Korn <uwelk@xhochy.com>
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

#ifndef PLAYLIST_P_H
#define PLAYLIST_P_H

#include "Playlist.h"

#include "playlist/RevisionQueueItem.h"

namespace Tomahawk
{

class PlaylistPrivate
{
    friend class DynamicPlaylist;
public:
    PlaylistPrivate( Playlist* q )
        : q_ptr( q )
    {
    }

    PlaylistPrivate( Playlist* q, const source_ptr& _author )
        : q_ptr( q )
        , source( _author )
        , lastmodified( 0 )
    {
    }

    PlaylistPrivate( Playlist* q, const source_ptr& _src,
                     const QString& _currentrevision,
                     const QString& _title,
                     const QString& _info,
                     const QString& _creator,
                     uint _createdOn,
                     bool _shared,
                     int _lastmod,
                     const QString& _guid )
        : q_ptr( q )
        , source( _src )
        , currentrevision( _currentrevision )
        , guid( _guid == "" ? uuid() : _guid )
        , title( _title )
        , info( _info )
        , creator( _creator )
        , lastmodified( _lastmod )
        , createdOn( _createdOn )
        , shared( _shared )
    {
    }

    PlaylistPrivate( Playlist* q, const source_ptr& _author,
                        const QString& _guid,
                        const QString& _title,
                        const QString& _info,
                        const QString& _creator,
                        bool _shared,
                        const QList< Tomahawk::plentry_ptr >& _entries )
        : q_ptr( q )
        , source( _author )
        , guid( _guid )
        , title( _title )
        , info ( _info )
        , creator( _creator )
        , lastmodified( 0 )
        , createdOn( 0 ) // will be set by db command
        , shared( _shared )
        , initEntries( _entries )
    {
    }

    Playlist* q_ptr;
    Q_DECLARE_PUBLIC ( Playlist )

private:
    QWeakPointer< Playlist > weakSelf;
    source_ptr source;
    QString currentrevision;
    QString guid;
    QString title;
    QString info;
    QString creator;
    unsigned int lastmodified;
    unsigned int createdOn;
    bool shared;
    bool loaded;

    QQueue<_detail::Closure*> queuedOps;
    QList< plentry_ptr > initEntries;
    QList< plentry_ptr > entries;

    QQueue<RevisionQueueItem> revisionQueue;
    QQueue<RevisionQueueItem> updateQueue;

    QList<PlaylistUpdaterInterface*> updaters;

    bool locallyChanged;
    bool deleted;
    bool busy;

    Tomahawk::playlistinterface_ptr playlistInterface;
};

}

#endif // PLAYLIST_P_H
