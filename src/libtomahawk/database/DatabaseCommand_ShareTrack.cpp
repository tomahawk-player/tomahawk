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

#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/InboxJobItem.h"
#include "playlist/InboxModel.h"
#include "network/Servent.h"

#include "Artist.h"
#include "Database.h"
#include "DatabaseImpl.h"
#include "PlaylistEntry.h"

namespace Tomahawk
{

DatabaseCommand_ShareTrack::DatabaseCommand_ShareTrack( QObject* parent )
    : DatabaseCommand_SocialAction( parent )
{}


DatabaseCommand_ShareTrack::DatabaseCommand_ShareTrack( const Tomahawk::trackdata_ptr& track,
                                                        const QString& recipientDbid,
                                                        QObject* parent )
    : DatabaseCommand_SocialAction( track, "Inbox", "", parent )
    , m_recipient( recipientDbid )
{}


QString
DatabaseCommand_ShareTrack::commandname() const
{
     return "sharetrack";
}


void
DatabaseCommand_ShareTrack::exec( DatabaseImpl* dbi )
{
    Q_ASSERT( !source().isNull() );

    QString myDbid = SourceList::instance()->getLocal()->nodeId();
    QString sourceDbid = source()->nodeId();
    if ( myDbid != m_recipient || sourceDbid == m_recipient )
        return;

    setComment( "true" /*unlistened*/ );

    DatabaseCommand_SocialAction::exec( dbi );
}


void
DatabaseCommand_ShareTrack::postCommitHook()
{
    if ( source()->isLocal() )
        Servent::instance()->triggerDBSync();
}


bool
DatabaseCommand_ShareTrack::doesMutates() const
{
    return true;
}


bool
DatabaseCommand_ShareTrack::singletonCmd() const
{
    return false;
}


bool
DatabaseCommand_ShareTrack::localOnly() const
{
    return false;
}


bool
DatabaseCommand_ShareTrack::groupable() const
{
    return true;
}


QString
DatabaseCommand_ShareTrack::recipient() const
{
    return m_recipient;
}


void
DatabaseCommand_ShareTrack::setRecipient( const QString& s )
{
     m_recipient = s;
}

}
