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

    typedef QSharedPointer<Collection> collection_ptr;
    typedef QSharedPointer<Playlist> playlist_ptr;
    typedef QSharedPointer<PlaylistEntry> plentry_ptr;
    typedef QSharedPointer<DynamicPlaylist> dynplaylist_ptr;
    typedef QSharedPointer<Query> query_ptr;
    typedef QSharedPointer<Result> result_ptr;
    typedef QSharedPointer<Source> source_ptr;
    typedef QSharedPointer<Artist> artist_ptr;
    typedef QSharedPointer<Album> album_ptr;

    // let's keep these typesafe, they are different kinds of GUID:
    typedef QString QID; //query id
    typedef QString RID; //result id

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
