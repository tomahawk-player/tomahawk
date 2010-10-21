#include "databasecommand_alltracks.h"

#include <QSqlQuery>

#include "databaseimpl.h"


void
DatabaseCommand_AllTracks::exec( DatabaseImpl* dbi )
{
    Q_ASSERT( !m_collection->source().isNull() );

    TomahawkSqlQuery query = dbi->newquery();
    QList<QVariant> tl;
    QString sql = QString(
            "SELECT file.id, artist.name, album.name, track.name, file.size, "
                   "file.duration, file.bitrate, file.url, file.source, file.mtime, file.mimetype, file_join.albumpos "
            "FROM file, artist, track, file_join "
            "LEFT OUTER JOIN album "
            "ON file_join.album = album.id "
            "WHERE file.id = file_join.file "
            "AND file_join.artist = artist.id "
            "AND file_join.track = track.id "
            "AND file.source %1 "
            ).arg( m_collection->source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( m_collection->source()->id() ) );

    query.prepare( sql );
    if( !query.exec() )
    {
        qDebug() << "ERROR:" << dbi->database().lastError().databaseText() << dbi->database().lastError().driverText();
    }

    int i = 0;
    while( query.next() )
    {
        QVariantMap t;
        QString url;
        url = query.value( 7 ).toString();
        if( m_collection->source()->isLocal() )
            t["url"] = url;
        else
            t["url"] = QString( "servent://%1\t%2" ).arg( m_collection->source()->userName() ).arg( url );

        t["id"] = QString( "%1" ).arg( query.value( 0 ).toInt() );
        t["artist"] = query.value( 1 ).toString();
        t["album"]  = query.value( 2 ).toString();
        t["track"]  = query.value( 3 ).toString();
        t["size"]     = query.value( 4 ).toInt();
        t["duration"] = query.value( 5 ).toInt();
        t["bitrate"]  = query.value( 6 ).toInt();
        t["lastmodified"] = query.value( 9 ).toInt();
        t["mimetype"] = query.value( 10 ).toString();
        t["albumpos"] = query.value( 11 ).toUInt();
        tl.append( t );

        if ( ++i % 1000 == 0 )
        {
            emit tracks( tl, m_collection );
            tl.clear();
        }
    }

    qDebug() << Q_FUNC_INFO << tl.length();

    if ( tl.count() )
        emit tracks( tl, m_collection );
    emit done( m_collection );
}
