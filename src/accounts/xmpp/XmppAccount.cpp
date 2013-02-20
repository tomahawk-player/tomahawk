/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "XmppAccount.h"
#include "XmppConfigWidget.h"
#include "sip/SipPlugin.h"
#include "XmppInfoPlugin.h"
#include "accounts/AccountConfigWidget.h"

#include <QtPlugin>

namespace Tomahawk
{

namespace Accounts
{

Account*
XmppAccountFactory::createAccount( const QString& accountId )
{
    return new XmppAccount( accountId.isEmpty() ? Tomahawk::Accounts::generateId( factoryId() ) : accountId );
}


XmppAccount::XmppAccount( const QString &accountId )
    : Account( accountId )
{
    setAccountServiceName( "Jabber (XMPP)" );
    setTypes( SipType );

    m_configWidget = QPointer< AccountConfigWidget >( new XmppConfigWidget( this, 0 ) );

    m_onlinePixmap = QPixmap( ":/xmpp-account/xmpp-icon.png" );
    m_offlinePixmap = QPixmap( ":/xmpp-account/xmpp-offline-icon.png" );
}


XmppAccount::~XmppAccount()
{
    delete m_xmppSipPlugin.data();
}

QPixmap
XmppAccount::icon() const
{
    if ( connectionState() == Connected )
        return m_onlinePixmap;
    return m_offlinePixmap;
}


void
XmppAccount::authenticate()
{
    if ( connectionState() != Account::Connected )
        sipPlugin()->connectPlugin();
}


void
XmppAccount::deauthenticate()
{
    if ( connectionState() != Account::Disconnected )
        sipPlugin()->disconnectPlugin();
}

bool
XmppAccount::isAuthenticated() const
{
    return m_xmppSipPlugin.data()->connectionState() == Account::Connected;
}


Account::ConnectionState
XmppAccount::connectionState() const
{
    if ( m_xmppSipPlugin.isNull() )
        return Account::Disconnected;

    return m_xmppSipPlugin.data()->connectionState();
}

void
XmppAccount::saveConfig()
{
    if ( !m_configWidget.isNull() )
        static_cast< XmppConfigWidget* >( m_configWidget.data() )->saveConfig();
}


InfoSystem::InfoPluginPtr
XmppAccount::infoPlugin()
{
    if( !m_xmppSipPlugin.isNull() )
        return m_xmppSipPlugin.data()->infoPlugin();

    return InfoSystem::InfoPluginPtr();
}


SipPlugin*
XmppAccount::sipPlugin()
{
    if ( m_xmppSipPlugin.isNull() )
    {
        m_xmppSipPlugin = QPointer< XmppSipPlugin >( new XmppSipPlugin( this ) );

        connect( m_xmppSipPlugin.data(), SIGNAL( stateChanged( Tomahawk::Accounts::Account::ConnectionState ) ), this, SIGNAL( connectionStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ) );
        connect( m_xmppSipPlugin.data(), SIGNAL( error( int, QString ) ), this, SIGNAL( error( int, QString ) ) );

        return m_xmppSipPlugin.data();
    }
    return m_xmppSipPlugin.data();
}


}

}

#ifndef GOOGLE_WRAPPER
Q_EXPORT_PLUGIN2( Tomahawk::Accounts::AccountFactory, Tomahawk::Accounts::XmppAccountFactory )
#endif
