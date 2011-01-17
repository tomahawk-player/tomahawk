#include "databasecommand_alltracks.h"

#include <QSqlQuery>

#include "databaseimpl.h"

void
DatabaseCommand_AllTracks::exec( DatabaseImpl* dbi )
{
    Q_ASSERT( !m_collection->source().isNull() );

    TomahawkSqlQuery query = dbi->newquery();
    QList<Tomahawk::query_ptr> ql;
    QString m_orderToken;

    switch ( m_sortOrder )
    {
        case 0:
            break;

        case Album:
            m_orderToken = "album.name, file_join.albumpos";
            break;

        case ModificationTime:
            m_orderToken = "file.mtime";
            break;

        case AlbumPosition:
            m_orderToken = "file_join.albumpos";
            break;
    }

    QString sql = QString(
            "SELECT file.id, artist.name, album.name, track.name, file.size, "
                   "file.duration, file.bitrate, file.url, file.source, file.mtime, file.mimetype, file_join.albumpos, artist.id, album.id, track.id "
            "FROM file, artist, track, file_join "
            "LEFT OUTER JOIN album "
            "ON file_join.album = album.id "
            "WHERE file.id = file_join.file "
            "AND file_join.artist = artist.id "
            "AND file_join.track = track.id "
            "AND file.source %1 "
            "%2 %3 "
            "%4 %5 %6"
            ).arg( m_collection->source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( m_collection->source()->id() ) )
             .arg( !m_artist ? QString() : QString( "AND artist.id = %1" ).arg( m_artist->id() ) )
             .arg( !m_album ? QString() : QString( "AND album.id = %1" ).arg( m_album->id() ) )
             .arg( m_sortOrder > 0 ? QString( "ORDER BY %1" ).arg( m_orderToken ) : QString() )
             .arg( m_sortDescending ? "DESC" : QString() )
             .arg( m_amount > 0 ? QString( "LIMIT 0, %1" ).arg( m_amount ) : QString() );

    query.prepare( sql );
    query.exec();

    int i = 0;
    while( query.next() )
    {
        QVariantMap t;
        QVariantMap attr;
        QString url;
        TomahawkSqlQuery attrQuery = dbi->newquery();

        url = query.value( 7 ).toString();
        if( m_collection->source()->isLocal() )
            t["url"] = url;
        else
            t["url"] = QString( "servent://%1\t%2" ).arg( m_collection->source()->userName() ).arg( url );

        t["id"] = QString( "%1" ).arg( query.value( 0 ).toInt() );
        t["artist"] = query.value( 1 ).toString();
        t["artistid"] = query.value( 12 ).toUInt();
        t["album"] = query.value( 2 ).toString();
        t["albumid"] = query.value( 13 ).toUInt();
        t["track"] = query.value( 3 ).toString();
        t["size"] = query.value( 4 ).toInt();
        t["duration"] = query.value( 5 ).toInt();
        t["bitrate"] = query.value( 6 ).toInt();
        t["mtime"] = query.value( 9 ).toInt();
        t["mimetype"] = query.value( 10 ).toString();
        t["albumpos"] = query.value( 11 ).toUInt();
        t["trackid"] = query.value( 14 ).toUInt();

        attrQuery.prepare( "SELECT k, v FROM track_attributes WHERE id = ?" );
        attrQuery.bindValue( 0, t["trackid"] );
        attrQuery.exec();
        while ( attrQuery.next() )
        {
            attr[ attrQuery.value( 0 ).toString() ] = attrQuery.value( 1 ).toString();
        }

        Tomahawk::query_ptr query = Tomahawk::query_ptr( new Tomahawk::Query( t ) );
        t.insert( "score", 1.0);

        Tomahawk::result_ptr result = Tomahawk::result_ptr( new Tomahawk::Result( t, m_collection ) );
        result->setAttributes( attr );

        QList<Tomahawk::result_ptr> results;
        results << result;
        query->addResults( results );

        ql << query;

        if ( ++i % 1000 == 0 )
        {
            emit tracks( ql, m_collection );
            ql.clear();
        }
    }

    qDebug() << Q_FUNC_INFO << ql.length();

    if ( ql.count() )
        emit tracks( ql, m_collection );
    emit done( m_collection );
}
