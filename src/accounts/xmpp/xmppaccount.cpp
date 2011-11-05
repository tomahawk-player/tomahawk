/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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
    , m_isAuthenticated( false )
{
    loadFromConfig( accountId );

    setAccountServiceName( "XMPP (Jabber)" );
    QSet< AccountType > types;
    types << SipType;
    setTypes( types );

    m_configWidget = QWeakPointer< XmppConfigWidget >( new XmppConfigWidget( this, 0 ) );
}


XmppAccount::~XmppAccount()
{

}


SipPlugin*
XmppAccount::sipPlugin()
{
    if ( m_xmppSipPlugin.isNull() )
    {
        m_xmppSipPlugin = QWeakPointer< XmppSipPlugin >( new XmppSipPlugin( this ) );
        return m_xmppSipPlugin.data();
    }
    return m_xmppSipPlugin.data();
}


}

}

Q_EXPORT_PLUGIN2( Tomahawk::Accounts::AccountFactory, Tomahawk::Accounts::XmppAccountFactory )
