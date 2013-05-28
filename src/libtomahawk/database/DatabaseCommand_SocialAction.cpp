/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christopher Reichert <creichert07@gmail.com>
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

#include "DatabaseCommand_SocialAction.h"

#include "database/Database.h"
#include "DatabaseImpl.h"
#include "network/Servent.h"
#include "utils/Logger.h"
#include "database/TomahawkSqlQuery.h"
#include "TrackData.h"

using namespace Tomahawk;


void
DatabaseCommand_SocialAction::postCommitHook()
{
    qDebug() << Q_FUNC_INFO;
    if ( source()->isLocal() )
    {
        Servent::instance()->triggerDBSync();
    }

    source()->reportSocialAttributesChanged( this );
}


void
DatabaseCommand_SocialAction::exec( DatabaseImpl* dbi )
{
    qDebug() << Q_FUNC_INFO;
    Q_ASSERT( !source().isNull() );

    TomahawkSqlQuery query = dbi->newquery();

    QVariant srcid = source()->isLocal() ? QVariant( QVariant::Int ) : source()->id();

    if ( m_artist.isNull() || m_title.isEmpty() || m_action.isEmpty() )
        return;

    int artid = dbi->artistId( m_artist, true );
    if ( artid < 1 )
        return;
    int trkid = dbi->trackId( artid, m_title, true );
    if ( trkid < 1 )
        return;

    // update if it already exists
    TomahawkSqlQuery find = dbi->newquery();
    find.prepare( QString( "SELECT id, k, v FROM social_attributes WHERE social_attributes.id = ? AND social_attributes.source %1 AND social_attributes.k = ?" ).arg( source()->isLocal() ? "IS NULL" : QString( "=%1" ).arg( source()->id() ) ) );
    find.addBindValue( trkid );
    find.addBindValue( m_action );
    if ( find.exec() && find.next() )
    {
        // update
        query.prepare( QString( "UPDATE social_attributes SET v = '%1', timestamp = %2 WHERE social_attributes.id = %3 AND social_attributes.source %4 AND social_attributes.k = '%5'" )
                               .arg( m_comment )
                               .arg( m_timestamp )
                               .arg( trkid )
                               .arg( source()->isLocal() ? "IS NULL" : QString( "=%1" ).arg( source()->id() ) )
                               .arg( m_action ) );
    }
    else
    {
        query.prepare( "INSERT INTO social_attributes(id, source, k, v, timestamp) "
                       "VALUES (?, ?, ?, ?, ?)" );

        query.bindValue( 0, trkid );
        query.bindValue( 1, srcid );
        query.bindValue( 2, m_action );
        query.bindValue( 3, m_comment );
        query.bindValue( 4, m_timestamp );
    }

    query.exec();
}

DatabaseCommand_SocialAction::DatabaseCommand_SocialAction( const Tomahawk::trackdata_ptr& track, QString action, QString comment, QObject* parent)
    : DatabaseCommandLoggable( parent ), m_track( track ), m_action( action )
{
    setSource( SourceList::instance()->getLocal() );

    setArtist( track->artist() );
    setTrack( track->track() );
    setComment( comment );
    setTimestamp( QDateTime::currentDateTime().toTime_t() );
}

