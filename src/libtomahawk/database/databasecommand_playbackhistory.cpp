#include "databasecommand_playbackhistory.h"

#include <QSqlQuery>

#include "databaseimpl.h"
#include "pipeline.h"

void
DatabaseCommand_PlaybackHistory::exec( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();
    QList<Tomahawk::query_ptr> ql;

    QString whereToken;
    if ( !source().isNull() )
    {
        whereToken = QString( "WHERE source %1" ).arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) );
    }

    QString sql = QString(
            "SELECT track, playtime, secs_played "
            "FROM playback_log "
            "%1" ).arg( whereToken );

    query.prepare( sql );
    query.exec();

    while( query.next() )
    {
        TomahawkSqlQuery query_track = dbi->newquery();

        QString sql = QString(
                "SELECT track.name, artist.name "
                "FROM track, artist "
                "WHERE artist.id = track.artist "
                "AND track.id = %1 "
                ).arg( query.value( 0 ).toUInt() );

        query_track.prepare( sql );
        query_track.exec();

        if ( query_track.next() )
        {
            QVariantMap m;
            m.insert( "track", query_track.value( 0 ).toString() );
            m.insert( "artist", query_track.value( 1 ).toString() );
            m.insert( "qid", uuid() );

            Tomahawk::query_ptr q( new Tomahawk::Query( m ) );
            ql << q;
        }
    }

    qDebug() << Q_FUNC_INFO << ql.length();

    if ( ql.count() )
    {
        Tomahawk::Pipeline::instance()->add( ql );
        emit tracks( ql );
    }
}
