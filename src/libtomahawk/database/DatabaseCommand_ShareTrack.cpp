/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Teo Mrnjavac <teo@kde.org>
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

#include "DatabaseCommand_ShareTrack.h"

#include "Artist.h"
#include "Database.h"
#include "DatabaseImpl.h"
#include "network/Servent.h"
#include "ViewManager.h"
#include "playlist/InboxModel.h"

DatabaseCommand_ShareTrack::DatabaseCommand_ShareTrack( QObject* parent )
    : DatabaseCommandLoggable( parent )
{}


DatabaseCommand_ShareTrack::DatabaseCommand_ShareTrack( const Tomahawk::query_ptr& query,
                                                        const QString& recipientDbid,
                                                        QObject* parent )
    : DatabaseCommandLoggable( parent )
    , m_query( query )
    , m_recipient( recipientDbid )
{
    setSource( SourceList::instance()->getLocal() );

    setArtist( query->artist() );
    setTrack( query->track() );
}


DatabaseCommand_ShareTrack::DatabaseCommand_ShareTrack( const Tomahawk::result_ptr& result,
                                                        const QString& recipientDbid,
                                                        QObject* parent )
    : DatabaseCommandLoggable( parent )
    , m_result( result )
    , m_recipient( recipientDbid )
{
    setSource( SourceList::instance()->getLocal() );

    setArtist( result->artist()->name() );
    setTrack( result->track() );
}

void
DatabaseCommand_ShareTrack::exec( DatabaseImpl* dbi )
{
    Q_ASSERT( !source().isNull() );
}

void
DatabaseCommand_ShareTrack::postCommitHook()
{
    if ( source()->isLocal() )
        Servent::instance()->triggerDBSync();

    if ( !m_query.isNull() )
        return;

    QString myDbid = SourceList::instance()->getLocal()->nodeId();
    QString sourceDbid = source()->nodeId();
    if ( myDbid != m_recipient || sourceDbid == m_recipient )
        return;

    //From here on, everything happens only on the recipient, and only if recipient!=source
    if ( !m_result.isNull() && m_query.isNull() )
    {
        m_query = m_result->toQuery();
    }
    else
    {
        m_query = Tomahawk::Query::get( m_artist, m_track, QString() );
    }

    if ( m_query.isNull() )
        return;

    QMetaObject::invokeMethod( ViewManager::instance()->inboxModel(),
                               "appendQuery",
                               Qt::QueuedConnection,
                               Q_ARG( const Tomahawk::query_ptr&, m_query ) );
}
