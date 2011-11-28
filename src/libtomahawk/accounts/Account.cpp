/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org?
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

#include "Account.h"

namespace Tomahawk
{

namespace Accounts
{

Account::Account( const QString& accountId )
    : QObject()
    , m_enabled( false )
    , m_autoConnect( false )
    , m_accountId( accountId )
{
    connect( this, SIGNAL( error( int, QString ) ), this, SLOT( onError( int,QString ) ) );
    connect( this, SIGNAL( connectionStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ) , this, SLOT( onConnectionStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ) );
}

QWidget*
Account::configurationWidget()
{
    return 0;
}


QWidget*
Account::aclWidget()
{
    return 0;
}


QIcon
Account::icon() const
{
    return QIcon();
}


bool
Account::canSelfAuthenticate() const
{
    return false;
}


void
Account::authenticate()
{
    return;
}


void
Account::deauthenticate()
{
    return;
}


bool
Account::isAuthenticated() const
{
    return false;
}


void
Account::refreshProxy()
{

}