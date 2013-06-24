/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */


#include "ACLJobStatusItem.h"

#include "infosystem/InfoSystem.h"
#include "ACLItemDelegate.h"

namespace Tomahawk {
namespace APIv2 {

ACLJobStatusItem::ACLJobStatusItem( const QSharedPointer<Api2User>& user )
    : JobStatusItem()
{
    m_user = user;
}


ACLJobStatusItem::~ACLJobStatusItem()
{

}


void
ACLJobStatusItem::createDelegate( QObject *parent )
{
    if ( m_delegate )
        return;

    m_delegate = new ACLItemDelegate( parent );

    // Display a system notification so that the user sees that he has to make a decision.
    Tomahawk::InfoSystem::InfoPushData pushData( "ACLJobStatusItem", Tomahawk::InfoSystem::InfoNotifyUser, tr( "Tomahawk needs you to decide whether an app is allowed to connect via the HTTP API." ), Tomahawk::InfoSystem::PushNoFlag );
    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( pushData );
}

} // namespace APIv2
} // namespace Tomahawk

