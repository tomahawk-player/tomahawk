#include "databasecommand_alltracks.h"

#include <QSqlQuery>

#include "databaseimpl.h"
#include "artist.h"
#include "album.h"


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
        Tomahawk::result_ptr result = Tomahawk::result_ptr( new Tomahawk::Result() );

        QVariantMap attr;
        TomahawkSqlQuery attrQuery = dbi->newquery();

        if( m_collection->source()->isLocal() )
            result->setUrl( query.value( 7 ).toString() );
        else
            result->setUrl( QString( "servent://%1\t%2" ).arg( m_collection->source()->userName() ).arg( query.value( 7 ).toString() ) );

        QString artist, track, album;
        artist = query.value( 1 ).toString();
        album = query.value( 2 ).toString();
        track = query.value( 3 ).toString();

        Tomahawk::query_ptr qry = Tomahawk::Query::get( artist, track, album );
        Tomahawk::artist_ptr artistptr = Tomahawk::Artist::get( query.value( 12 ).toUInt(), artist, m_collection );
        Tomahawk::album_ptr albumptr = Tomahawk::Album::get( query.value( 13 ).toUInt(), album, artistptr, m_collection );

        result->setId( query.value( 14 ).toUInt() );
        result->setArtist( artistptr );
        result->setAlbum( albumptr );
        result->setTrack( query.value( 3 ).toString() );
        result->setSize( query.value( 4 ).toUInt() );
        result->setDuration( query.value( 5 ).toUInt() );
        result->setBitrate( query.value( 6 ).toUInt() );
        result->setModificationTime( query.value( 9 ).toUInt() );
        result->setMimetype( query.value( 10 ).toString() );
        result->setAlbumPos( query.value( 11 ).toUInt() );
        result->setScore( 1.0 );
        result->setCollection( m_collection );

        attrQuery.prepare( "SELECT k, v FROM track_attributes WHERE id = ?" );
        attrQuery.bindValue( 0, result->dbid() );
        attrQuery.exec();
        while ( attrQuery.next() )
        {
            attr[ attrQuery.value( 0 ).toString() ] = attrQuery.value( 1 ).toString();
        }

        result->setAttributes( attr );

        QList<Tomahawk::result_ptr> results;
        results << result;
        qry->addResults( results );

        ql << qry;
    }

    qDebug() << Q_FUNC_INFO << ql.length();

    emit tracks( ql, m_collection );
    emit done( m_collection );
}
