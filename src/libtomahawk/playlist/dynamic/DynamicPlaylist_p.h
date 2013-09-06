/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef DYNAMICPLAYLIST_P_H
#define DYNAMICPLAYLIST_P_H

#include "DynamicPlaylist.h"

#include "playlist/RevisionQueueItem.h"

#include "Playlist_p.h"

namespace Tomahawk
{

/**
 * Subclass of playlist that adds the information needed to store a dynamic playlist.
 *  It uses normal PlaylistEntries but also has a mode, a generator, and a list of controls
*/

struct DynQueueItem : RevisionQueueItem
{
    QString type;
    QList <dyncontrol_ptr> controls;
    int mode;

    DynQueueItem( const QString& nRev, const QString& oRev, const QString& typ, const QList< dyncontrol_ptr >& ctrls,  int m, const QList< plentry_ptr >& e, bool latest ) :
        RevisionQueueItem( nRev, oRev, e, latest ), type( typ ), controls( ctrls ), mode( m ) {}
};

class DynamicPlaylistPrivate : public PlaylistPrivate
{
public:
    DynamicPlaylistPrivate( DynamicPlaylist* q, const source_ptr& _author )
        : PlaylistPrivate( q, _author )
    {
    }


    DynamicPlaylistPrivate( DynamicPlaylist* q, const source_ptr& _src,
                     const QString& _currentrevision,
                     const QString& _title,
                     const QString& _info,
                     const QString& _creator,
                     uint _createdOn,
                     bool _shared,
                     int _lastmod,
                     const QString& _guid,
                     bool _autoload )
        : PlaylistPrivate( q, _src, _currentrevision, _title, _info, _creator, _createdOn, _shared, _lastmod, _guid )
        , autoLoad( _autoload )
    {
    }

    Q_DECLARE_PUBLIC( DynamicPlaylist )

private:
    QWeakPointer< DynamicPlaylist > weakSelf;

    geninterface_ptr generator;
    bool autoLoad;

    QQueue<DynQueueItem> revisionQueue;
};

} // Tomahawk;

#endif // DYNAMICPLAYLIST_P_H
