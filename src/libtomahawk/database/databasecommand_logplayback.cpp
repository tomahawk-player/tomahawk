/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "databasecommand_logplayback.h"

#include <QSqlQuery>

#include "Collection.h"
#include "database/database.h"
#include "DatabaseImpl.h"
#include "network/Servent.h"
#include "utils/logger.h"

#define STARTED_THRESHOLD 600   // Don't advertise tracks older than X seconds as currently playing
#define FINISHED_THRESHOLD 10   // Don't store tracks played less than X seconds in the playback log
#define SUBMISSION_THRESHOLD 20 // Don't broadcast playback logs when a track was played less than X seconds

using namespace Tomahawk;


void
DatabaseCommand_LogPlayback::postCommitHook()
{
    connect( this, SIGNAL( trackPlaying( Tomahawk::query_ptr, unsigned int ) ),
             source().data(), SLOT( onPlaybackStarted( Tomahawk::query_ptr, unsigned int ) ), Qt::QueuedConnection );
    connect( this, SIGNAL( trackPlayed( Tomahawk::query_ptr ) ),
             source().data(), SLOT( onPlaybackFinished( Tomahawk::query_ptr ) ), Qt::QueuedConnection );

    Tomahawk::query_ptr q;
    if ( !m_result.isNull() )
    {
        q = m_result->toQuery();
    }
    else
    {
        // do not auto resolve this track
        q = Tomahawk::Query::get( m_artist, m_track, QString() );
    }
    q->setPlayedBy( source(), m_playtime );

    if ( m_action == Finished )
    {
        emit trackPlayed( q );
    }
    // if the play time is more than 10 minutes in the past, ignore
    else if ( m_action == Started && QDateTime::fromTime_t( playtime() ).secsTo( QDateTime::currentDateTime() ) < STARTED_THRESHOLD )
    {
        emit trackPlaying( q, m_trackDuration );
    }

    if ( source()->isLocal() )
    {
        Servent::instance()->triggerDBSync();
    }
}


void
DatabaseCommand_LogPlayback::exec( DatabaseImpl* dbi )
{
    Q_ASSERT( !source().isNull() );

    if ( m_action != Finished )
        return;
    if ( m_secsPlayed < FINISHED_THRESHOLD )
        return;

    TomahawkSqlQuery query = dbi->newquery();
    query.prepare( "INSERT INTO playback_log(source, track, playtime, secs_played) "
                   "VALUES (?, ?, ?, ?)" );

    QVariant srcid = source()->isLocal() ? QVariant( QVariant::Int ) : source()->id();
    qDebug() << "Logging playback of" << m_artist << "-" << m_track << "for source" << srcid;
    query.bindValue( 0, srcid );

    // If there's no artist, becuase it's a resolver result with bad metadata for example, don't save it
    bool autoCreate = m_artist.isEmpty();
    int artid = dbi->artistId( m_artist, autoCreate );
    if( artid < 1 )
        return;

    autoCreate = true; // artistId overwrites autoCreate (reference)
    int trkid = dbi->trackId( artid, m_track, autoCreate );
    if( trkid < 1 )
        return;

    query.bindValue( 1, trkid );
    query.bindValue( 2, m_playtime );
    query.bindValue( 3, m_secsPlayed );

    query.exec();
}


bool
DatabaseCommand_LogPlayback::localOnly() const
{
    if ( m_action == Finished )
        return m_secsPlayed < SUBMISSION_THRESHOLD;

    return false;
}

