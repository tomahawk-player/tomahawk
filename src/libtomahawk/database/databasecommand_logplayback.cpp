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

    if( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}


void
DatabaseCommand_LogPlayback::exec( DatabaseImpl* dbi )
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( !source().isNull() );

    TomahawkSqlQuery query = dbi->newquery();
    query.prepare( "INSERT INTO playback_log(source,track,playtime,secs_played) "
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
