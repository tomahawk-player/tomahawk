/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
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

#ifndef TYPEDEFS_H
#define TYPEDEFS_H

#include <QSharedPointer>
#include <QUuid>

namespace Tomahawk
{
    class Artist;
    class Album;
    class Collection;
    class Playlist;
    class PlaylistEntry;
    class DynamicPlaylist;
    class Query;
    class Result;
    class Source;
    class DynamicControl;
    class GeneratorInterface;

    typedef QSharedPointer<Collection> collection_ptr;
    typedef QSharedPointer<Playlist> playlist_ptr;
    typedef QSharedPointer<PlaylistEntry> plentry_ptr;
    typedef QSharedPointer<DynamicPlaylist> dynplaylist_ptr;
    typedef QSharedPointer<Query> query_ptr;
    typedef QSharedPointer<Result> result_ptr;
    typedef QSharedPointer<Source> source_ptr;
    typedef QSharedPointer<Artist> artist_ptr;
    typedef QSharedPointer<Album> album_ptr;
    
    typedef QSharedPointer<DynamicControl> dyncontrol_ptr;
    typedef QSharedPointer<GeneratorInterface> geninterface_ptr;
    
    // let's keep these typesafe, they are different kinds of GUID:
    typedef QString QID; //query id
    typedef QString RID; //result id
    
    
    enum GeneratorMode {
        OnDemand = 0,
        Static
    };

}; // ns

typedef int AudioErrorCode;

// creates 36char ascii guid without {} around it
inline static QString uuid()
{
    // kinda lame, but
    QString q = QUuid::createUuid();
    q.remove( 0, 1 );
    q.chop( 1 );
    return q;
}

#endif // TYPEDEFS_H
