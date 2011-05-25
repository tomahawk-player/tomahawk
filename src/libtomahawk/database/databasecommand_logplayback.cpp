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

#include "databasecommand_logplayback.h"

#include <QSqlQuery>

#include "collection.h"
#include "database/database.h"
#include "databaseimpl.h"
#include "network/servent.h"

using namespace Tomahawk;


void
DatabaseCommand_LogPlayback::postCommitHook()
{
    qDebug() << Q_FUNC_INFO;

    connect( this, SIGNAL( trackPlaying( Tomahawk::query_ptr ) ),
             source().data(), SLOT( onPlaybackStarted( Tomahawk::query_ptr ) ), Qt::QueuedConnection );
    connect( this, SIGNAL( trackPlayed( Tomahawk::query_ptr ) ),
             source().data(), SLOT( onPlaybackFinished( Tomahawk::query_ptr ) ), Qt::QueuedConnection );

    // do not auto resolve this track
    Tomahawk::query_ptr q = Tomahawk::Query::get( m_artist, m_track, QString() );

    if ( m_action == Finished )
    {
        emit trackPlayed( q );
    }
    else if ( m_action == Started )
    {
        emit trackPlaying( q );
    }

    if ( source()->isLocal() )
    {
        Servent::instance()->triggerDBSync();
    }
}


void
DatabaseCommand_LogPlayback::exec( DatabaseImpl* dbi )
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( !source().isNull() );

    if ( m_action != Finished )
        return;
    if ( m_secsPlayed < 10 )
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


bool
DatabaseCommand_LogPlayback::localOnly() const
{
    if ( m_action == Finished )
        return m_secsPlayed < 20;

    return false;
}

