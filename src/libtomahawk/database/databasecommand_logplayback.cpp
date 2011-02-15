#include "databasecommand_logplayback.h"

#include <QSqlQuery>

#include "collection.h"
#include "database/database.h"
#include "databaseimpl.h"
#include "network/servent.h"

using namespace Tomahawk;


// After changing a collection, we need to tell other bits of the system:
void
DatabaseCommand_LogPlayback::postCommitHook()
{
    qDebug() << Q_FUNC_INFO;

    connect( this, SIGNAL( trackPlaying( Tomahawk::query_ptr ) ),
             source().data(), SIGNAL( playbackStarted( Tomahawk::query_ptr ) ), Qt::QueuedConnection );
    connect( this, SIGNAL( trackPlayed( Tomahawk::query_ptr ) ),
             source().data(), SIGNAL( playbackFinished( Tomahawk::query_ptr ) ), Qt::QueuedConnection );

    QVariantMap m;
    m.insert( "track", m_track );
    m.insert( "artist", m_artist );
    m.insert( "qid", uuid() );
    Tomahawk::query_ptr q = Tomahawk::Query::get( m );

    if ( m_action == Finished )
    {
        emit trackPlayed( q );
    }
    else if ( m_action == Started )
    {
        emit trackPlaying( q );
    }

    if( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}


void
DatabaseCommand_LogPlayback::exec( DatabaseImpl* dbi )
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( !source().isNull() );

    if ( m_action != Finished )
        return;

    TomahawkSqlQuery query = dbi->newquery();
    query.prepare( "INSERT INTO playback_log(source, track, playtime, secs_played) "
                   "VALUES (?, ?, ?, ?)" );

    QVariant srcid = source()->isLocal() ? QVariant( QVariant::Int ) : source()->id();

    qDebug() << "Logging playback of" << m_artist << "-" << m_track << "for source" << srcid;

    query.bindValue( 0, srcid );

    bool isnew;
    int artid = dbi->artistId( m_artist, isnew );
    if( artid < 1 )
        return;

    int trkid = dbi->trackId( artid, m_track, isnew );
    if( trkid < 1 )
        return;

    query.bindValue( 1, trkid );
    query.bindValue( 2, m_playtime );
    query.bindValue( 3, m_secsPlayed );

    query.exec();
}
