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
#include "accounts/twitter/tomahawkoauthtwitter.h"

#include "sip/SipPlugin.h"

#include <QTweetLib/qtweetaccountverifycredentials.h>
#include <QTweetLib/qtweetuser.h>
#include <QTweetLib/qtweetstatus.h>
#include <QTweetLib/qtweetusershow.h>

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

    m_twitterAuth = QWeakPointer< TomahawkOAuthTwitter >( new TomahawkOAuthTwitter( TomahawkUtils::nam(), this ) );
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


void
TwitterAccount::authenticate()
{
    tDebug() << Q_FUNC_INFO << "credentials: " << credentials().keys();

    if ( credentials[ "oauthtoken" ].toString().isEmpty() || credentials()[ "oauthtokensecret" ].toString().isEmpty() )
    {
        qDebug() << "TwitterSipPlugin has empty Twitter credentials; not connecting";
        return;
    }

    if ( refreshTwitterAuth() )
    {
        QTweetAccountVerifyCredentials *credVerifier = new QTweetAccountVerifyCredentials( m_twitterAuth.data(), this );
        connect( credVerifier, SIGNAL( parsedUser( const QTweetUser & ) ), SLOT( connectAuthVerifyReply( const QTweetUser & ) ) );
        credVerifier->verify();
    }
}


void
TwitterAccount::deauthenticate()
{
    if ( sipPlugin() )
        sipPlugin()->disconnectPlugin();

    m_isAuthenticated = false;
    emit nowDeauthenticated();
}



bool
TwitterAccount::refreshTwitterAuth()
{
    qDebug() << Q_FUNC_INFO << " begin";
    if( !m_twitterAuth.isNull() )
        delete m_twitterAuth.data();

    Q_ASSERT( TomahawkUtils::nam() != 0 );
    qDebug() << Q_FUNC_INFO << " with nam " << TomahawkUtils::nam();
    m_twitterAuth = QWeakPointer< TomahawkOAuthTwitter >( new TomahawkOAuthTwitter( TomahawkUtils::nam(), this ) );

    if( m_twitterAuth.isNull() )
      return false;

    m_twitterAuth.data()->setOAuthToken( m_credentials[ "oauthtoken" ].toString().toLatin1() );
    m_twitterAuth.data()->setOAuthTokenSecret( m_credentials[ "oauthtokensecret" ].toString().toLatin1() );

    return true;
}


void
TwitterAccount::connectAuthVerifyReply( const QTweetUser &user )
{
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
        emit nowAuthenticated( m_twitterAuth, user );
    }
}


void
TwitterAccount::refreshProxy()
{
    //FIXME: Could this cause a race condition if a client is threaded?
    if ( !m_twitterAuth.isNull() )
        m_twitterAuth.data()->setNetworkAccessManager( TomahawkUtils::nam() );
}


}

}

Q_EXPORT_PLUGIN2( Tomahawk::Accounts::AccountFactory, Tomahawk::Accounts::TwitterAccountFactory )