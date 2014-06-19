/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014,      Uwe L. Korn <uwelk@xhochy.com>
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

#include "StatResponseHandler.h"

#include "Api_v1.h"

StatResponseHandler::StatResponseHandler( Api_v1* parent, QxtWebRequestEvent* event )
    : QObject( parent )
    , m_parent( parent )
    , m_storedEvent( event )
{
}


void
StatResponseHandler::statResult( const QString& clientToken, const QString& name, bool valid )
{
    Q_UNUSED( clientToken )
    Q_UNUSED( name )

    Q_ASSERT( m_storedEvent );
    if ( !m_storedEvent )
        return;

    QVariantMap m;
    m.insert( "name", "playdar" );
    m.insert( "version", "0.1.1" ); // TODO (needs to be >=0.1.1 for JS to work)
    m.insert( "authenticated", valid ); // TODO
    m.insert( "capabilities", QVariantList() );
    m_parent->sendJSON( m, m_storedEvent );

    deleteLater();
}
