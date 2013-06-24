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

#include "Api2User.h"

Api2User::Api2User( const QString& name )
    : QObject( 0 )
    , m_aclDecision( None )
    , m_name( name )
{
}

Api2User::ACLDecision
Api2User::aclDecision() const
{
    return m_aclDecision;
}


QString
Api2User::name() const
{
    return m_name;
}


QSslKey
Api2User::pubkey() const
{
    return m_pubkey;
}


void
Api2User::setPubkey( const QSslKey &pubkey )
{
    m_pubkey = pubkey;
}


QString
Api2User::clientDescription() const
{
    return m_clientDescription;
}


void
Api2User::setClientDescription( const QString& clientDescription )
{
    m_clientDescription = clientDescription;
}


QString
Api2User::clientName() const
{
    return m_clientName;
}


void
Api2User::setClientName( const QString& clientName )
{
    m_clientName = clientName;
}
