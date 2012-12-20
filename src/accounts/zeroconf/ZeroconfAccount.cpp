/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "ZeroconfAccount.h"

#include "sip/SipPlugin.h"
#include "Zeroconf.h"
#include "Source.h"

#include <QtPlugin>

using namespace Tomahawk;
using namespace Accounts;

QPixmap* s_icon = 0;

ZeroconfFactory::ZeroconfFactory()
{
#ifndef ENABLE_HEADLESS
    if ( s_icon == 0 )
        s_icon = new QPixmap( ":/zeroconf-icon.png" );
#endif
}


ZeroconfFactory::~ZeroconfFactory()
{
    if ( s_icon )
    {
        delete s_icon;
        s_icon = 0;
    }
}


Account*
ZeroconfFactory::createAccount( const QString& pluginId )
{
    return new ZeroconfAccount( pluginId.isEmpty() ? generateId( factoryId() ) : pluginId );
}

QPixmap
ZeroconfFactory::icon() const
{
    return *s_icon;
}


ZeroconfAccount::ZeroconfAccount( const QString& accountId )
    : Account( accountId )
{
    setAccountServiceName( tr( "Local Network" ) );
    setAccountFriendlyName( tr( "Local Network" ) );

    setTypes( SipType );
}

ZeroconfAccount::~ZeroconfAccount()
{

}

QPixmap
ZeroconfAccount::icon() const
{
    return *s_icon;
}


void
ZeroconfAccount::authenticate()
{
    if ( !isAuthenticated() )
        sipPlugin()->connectPlugin();
}


void
ZeroconfAccount::deauthenticate()
{
    if ( isAuthenticated() )
        sipPlugin()->disconnectPlugin();
}


bool
ZeroconfAccount::isAuthenticated() const
{
    return connectionState() == Connected;
}


Account::ConnectionState
ZeroconfAccount::connectionState() const
{
    if ( m_sipPlugin.isNull() )
        return Disconnected;

    // TODO can we get called before sipPlugin()?
    return m_sipPlugin.data()->connectionState();
}


SipPlugin*
ZeroconfAccount::sipPlugin()
{
    if ( m_sipPlugin.isNull() )
        m_sipPlugin = QWeakPointer< ZeroconfPlugin >( new ZeroconfPlugin( this ) );

    return m_sipPlugin.data();
}


Q_EXPORT_PLUGIN2( Tomahawk::Accounts::AccountFactory, Tomahawk::Accounts::ZeroconfFactory )
