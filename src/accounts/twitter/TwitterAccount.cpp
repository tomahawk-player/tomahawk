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

#include "TwitterAccount.h"
#include "TwitterConfigWidget.h"
#include "accounts/twitter/TomahawkOAuthTwitter.h"
#include "libtomahawk/infosystem/InfoSystem.h"
#include "utils/Logger.h"
#include "sip/SipPlugin.h"

#include <QTweetLib/qtweetaccountverifycredentials.h>
#include <QTweetLib/qtweetuser.h>
#include <QTweetLib/qtweetstatus.h>
#include <QTweetLib/qtweetusershow.h>

#include <QtCore/QtPlugin>
#include <QTimer>

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
    , m_isAuthenticating( false )
{
    setAccountServiceName( "Twitter" );
    setTypes( AccountTypes( StatusPushType ) );

    qDebug() << "Got cached peers:" << configuration() << configuration()[ "cachedpeers" ];

    m_configWidget = QPointer< TwitterConfigWidget >( new TwitterConfigWidget( this, 0 ) );
    connect( m_configWidget.data(), SIGNAL( twitterAuthed( bool ) ), SLOT( configDialogAuthedSignalSlot( bool ) ) );

    m_twitterAuth = QPointer< TomahawkOAuthTwitter >( new TomahawkOAuthTwitter( TomahawkUtils::nam(), this ) );

    m_onlinePixmap = QPixmap( ":/twitter-account/twitter-icon.png" );
    m_offlinePixmap = QPixmap( ":/twitter-account/twitter-offline-icon.png" );
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


Account::ConnectionState
TwitterAccount::connectionState() const
{
//    if ( m_twitterSipPlugin.isNull() )
        return Account::Disconnected;

//    return m_twitterSipPlugin.data()->connectionState();
}

SipPlugin*
TwitterAccount::sipPlugin()
{
//    if ( m_twitterSipPlugin.isNull() )
//    {
//        qDebug() << "CHECKING:" << configuration() << configuration()[ "cachedpeers" ];
//        m_twitterSipPlugin = QPointer< TwitterSipPlugin >( new TwitterSipPlugin( this ) );

//        connect( m_twitterSipPlugin.data(), SIGNAL( stateChanged( Tomahawk::Accounts::Account::ConnectionState ) ), this, SIGNAL( connectionStateChanged( Tomahawk::Accounts::Account::ConnectionState ) ) );
//        return m_twitterSipPlugin.data();
//    }
//    return m_twitterSipPlugin.data();
    return 0;
}


Tomahawk::InfoSystem::InfoPluginPtr
TwitterAccount::infoPlugin()
{
    if ( m_twitterInfoPlugin.isNull() )
        m_twitterInfoPlugin = QPointer< Tomahawk::InfoSystem::TwitterInfoPlugin >( new Tomahawk::InfoSystem::TwitterInfoPlugin( this ) );

    return Tomahawk::InfoSystem::InfoPluginPtr( m_twitterInfoPlugin.data() );
}


void
TwitterAccount::authenticate()
{
    // Since we need to have a chance for deletion (via the infosystem) to work on the info plugin, we put this on the event loop
    tDebug() << Q_FUNC_INFO;
    QTimer::singleShot( 0, this, SLOT( authenticateSlot() ) );
}


void
TwitterAccount::authenticateSlot()
{
    tDebug() << Q_FUNC_INFO;
    if ( m_twitterInfoPlugin.isNull() )
    {
        if ( infoPlugin() && Tomahawk::InfoSystem::InfoSystem::instance()->workerThread() )
        {
            infoPlugin().data()->moveToThread( Tomahawk::InfoSystem::InfoSystem::instance()->workerThread().data() );
            Tomahawk::InfoSystem::InfoSystem::instance()->addInfoPlugin( infoPlugin() );
        }
    }

    if ( m_isAuthenticating )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Already authenticating";
        return;
    }

    tDebug() << Q_FUNC_INFO << "credentials: " << credentials().keys();

    if ( credentials()[ "oauthtoken" ].toString().isEmpty() || credentials()[ "oauthtokensecret" ].toString().isEmpty() )
    {
        tDebug() << Q_FUNC_INFO << "TwitterSipPlugin has empty Twitter credentials; not connecting";
        return;
    }

    if ( refreshTwitterAuth() )
    {
        m_isAuthenticating = true;
        tDebug() << Q_FUNC_INFO << "Verifying credentials";
        QTweetAccountVerifyCredentials *credVerifier = new QTweetAccountVerifyCredentials( m_twitterAuth.data(), this );
        connect( credVerifier, SIGNAL( parsedUser( const QTweetUser & ) ), SLOT( connectAuthVerifyReply( const QTweetUser & ) ) );
        credVerifier->verify();
    }
}


void
TwitterAccount::deauthenticate()
{
    tDebug() << Q_FUNC_INFO;

//    if ( m_twitterSipPlugin )
//        sipPlugin()->disconnectPlugin();

    if ( m_twitterInfoPlugin )
        Tomahawk::InfoSystem::InfoSystem::instance()->removeInfoPlugin( m_twitterInfoPlugin.data() );

    m_isAuthenticated = false;
    m_isAuthenticating = false;

    emit nowDeauthenticated();
}



bool
TwitterAccount::refreshTwitterAuth()
{
    qDebug() << Q_FUNC_INFO << " begin";
    if( !m_twitterAuth.isNull() )
        delete m_twitterAuth.data();

    Q_ASSERT( TomahawkUtils::nam() != 0 );
    tDebug() << Q_FUNC_INFO << " with nam " << TomahawkUtils::nam();
    m_twitterAuth = QPointer< TomahawkOAuthTwitter >( new TomahawkOAuthTwitter( TomahawkUtils::nam(), this ) );

    if( m_twitterAuth.isNull() )
      return false;

    m_twitterAuth.data()->setOAuthToken( credentials()[ "oauthtoken" ].toString().toLatin1() );
    m_twitterAuth.data()->setOAuthTokenSecret( credentials()[ "oauthtokensecret" ].toString().toLatin1() );

    return true;
}


void
TwitterAccount::connectAuthVerifyReply( const QTweetUser &user )
{
    m_isAuthenticating = false;
    if ( user.id() == 0 )
    {
        qDebug() << "TwitterAccount could not authenticate to Twitter";
        deauthenticate();
    }
    else
    {
        tDebug() << "TwitterAccount successfully authenticated to Twitter as user " << user.screenName();
        QVariantHash config = configuration();
        config[ "screenname" ] = user.screenName();
        setConfiguration( config );
        sync();

//        sipPlugin()->connectPlugin();

        m_isAuthenticated = true;
        emit nowAuthenticated( m_twitterAuth, user );
    }
}


QPixmap
TwitterAccount::icon() const
{
    if ( connectionState() == Connected )
        return m_onlinePixmap;
    return m_offlinePixmap;
}


}

}

Q_EXPORT_PLUGIN2( Tomahawk::Accounts::AccountFactory, Tomahawk::Accounts::TwitterAccountFactory )
