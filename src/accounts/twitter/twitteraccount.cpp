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

#include "twitteraccount.h"

#include "twitterconfigwidget.h"

#include "sip/SipPlugin.h"

#include <QtCore/QtPlugin>

namespace Tomahawk
{

namespace Accounts
{

Account*
TwitterAccountFactory::createAccount( const QString& accountId )
{
    return new TwitterAccount( accountId.isEmpty() ? Tomahawk::Accounts::generateId( factoryId() ) : accountId );
}


TwitterAccount::TwitterAccount( const QString &accountId )
    : Account( accountId )
    , m_isAuthenticated( false )
{
    loadFromConfig( accountId );

    setAccountServiceName( "Twitter" );
    QSet< AccountType > types;
    types << InfoType << SipType;
    setTypes( types );
    
    m_configWidget = QWeakPointer< TwitterConfigWidget >( new TwitterConfigWidget( this, 0 ) );
    connect( m_configWidget.data(), SIGNAL( twitterAuthed( bool ) ), SLOT( configDialogAuthedSignalSlot( bool ) ) );
}


TwitterAccount::~TwitterAccount()
{

}


void
TwitterAccount::configDialogAuthedSignalSlot( bool authed )
{
    tDebug() << Q_FUNC_INFO;
    m_isAuthenticated = authed;
    if ( !credentials()[ "username" ].toString().isEmpty() )
        setAccountFriendlyName( QString( "@%1" ).arg( credentials()[ "username" ].toString() ) );
    syncConfig();
    emit configurationChanged();
}


SipPlugin*
TwitterAccount::sipPlugin()
{
    if ( m_twitterSipPlugin.isNull() )
    {
        m_twitterSipPlugin = QWeakPointer< TwitterSipPlugin >( new TwitterSipPlugin( this ) );
        return m_twitterSipPlugin.data();
    }
    return m_twitterSipPlugin.data();
}


}

}

Q_EXPORT_PLUGIN2( Tomahawk::Accounts::AccountFactory, Tomahawk::Accounts::TwitterAccountFactory )