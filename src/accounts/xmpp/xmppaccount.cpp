/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "xmppaccount.h"
#include "xmppconfigwidget.h"
#include "sip/SipPlugin.h"
#include "ui_xmppconfigwidget.h"

#include <QtCore/QtPlugin>

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
    setAccountServiceName( "XMPP (Jabber)" );
    setTypes( SipType );

    m_configWidget = QWeakPointer< QWidget >( new XmppConfigWidget( this, 0 ) );
}


XmppAccount::~XmppAccount()
{
    delete m_xmppSipPlugin.data();
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
    return m_xmppSipPlugin.data()->connectionState();
}

void
XmppAccount::saveConfig()
{
    if ( !m_configWidget.isNull() )
        static_cast< XmppConfigWidget* >( m_configWidget.data() )->saveConfig();
}


SipPlugin*
XmppAccount::sipPlugin()
{
    if ( m_xmppSipPlugin.isNull() )
    {
        m_xmppSipPlugin = QWeakPointer< XmppSipPlugin >( new XmppSipPlugin( this ) );

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
