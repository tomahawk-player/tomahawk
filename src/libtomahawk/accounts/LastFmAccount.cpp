/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
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

#include "LastFmAccount.h"
#include "LastFmConfig.h"

#include "infosystem/infosystem.h"
#include "infosystem/infoplugins/generic/lastfmplugin.h"
#include "utils/tomahawkutils.h"
#include "resolvers/qtscriptresolver.h"
#include "AtticaManager.h"
#include "pipeline.h"
#include "accounts/AccountManager.h"

using namespace Tomahawk;
using namespace InfoSystem;
using namespace Accounts;

LastFmAccountFactory::LastFmAccountFactory()
{
    m_icon.load( RESPATH "images/lastfm-icon.png" );
}


Account*
LastFmAccountFactory::createAccount( const QString& accountId )
{
    return new LastFmAccount( accountId.isEmpty() ? generateId( factoryId() ) : accountId );
}


QPixmap
LastFmAccountFactory::icon() const
{
    return m_icon;
}


LastFmAccount::LastFmAccount( const QString& accountId )
    : CustomAtticaAccount( accountId )
{
    m_infoPlugin = QWeakPointer< LastFmPlugin >( new LastFmPlugin( this ) );

    setAccountFriendlyName( "Last.Fm" );
    m_icon.load( RESPATH "images/lastfm-icon.png" );

    AtticaManager::instance()->registerCustomAccount( "lastfm", this );

    connect( AtticaManager::instance(), SIGNAL( resolverInstalled( QString ) ), this, SLOT( resolverInstalled( QString ) ) );

    const Attica::Content res = AtticaManager::instance()->resolverForId( "lastfm" );
    const AtticaManager::ResolverState state = AtticaManager::instance()->resolverState( res );

    if ( state == AtticaManager::Installed )
    {
        hookupResolver();
    }
}


LastFmAccount::~LastFmAccount()
{
    delete m_infoPlugin.data();
    delete m_resolver.data();
}


void
LastFmAccount::authenticate()
{
    const Attica::Content res = AtticaManager::instance()->resolverForId( "lastfm" );
    const AtticaManager::ResolverState state = AtticaManager::instance()->resolverState( res );

    qDebug() << "Last.FM account authenticating...";
    if ( m_resolver.isNull() && state == AtticaManager::Installed )
    {
        hookupResolver();
    }
    else if ( m_resolver.isNull() )
    {
        qDebug() << "Got null resolver but asked to authenticate, so installing i we have one from attica:" << res.isValid() << res.id();
        if ( res.isValid() && !res.id().isEmpty() )
            AtticaManager::instance()->installResolver( res, false );
    }
    else
    {
        m_resolver.data()->start();
    }

    emit connectionStateChanged( connectionState() );
}


void
LastFmAccount::deauthenticate()
{
    if ( !m_resolver.isNull() && m_resolver.data()->running() )
        m_resolver.data()->stop();

    emit connectionStateChanged( connectionState() );
}


QWidget*
LastFmAccount::configurationWidget()
{
    if ( m_configWidget.isNull() )
        m_configWidget = QWeakPointer<LastFmConfig>( new LastFmConfig( this ) );

    return m_configWidget.data();
}


Account::ConnectionState
LastFmAccount::connectionState() const
{
    return (!m_resolver.isNull() && m_resolver.data()->running()) ? Account::Connected : Account::Disconnected;
}


QPixmap
LastFmAccount::icon() const
{
    return m_icon;
}


InfoPlugin*
LastFmAccount::infoPlugin()
{
    return m_infoPlugin.data();
}

bool
LastFmAccount::isAuthenticated() const
{
    return !m_resolver.isNull() && m_resolver.data()->running();
}


void
LastFmAccount::saveConfig()
{
    if ( !m_configWidget.isNull() )
    {
        setUsername( m_configWidget.data()->username() );
        setPassword( m_configWidget.data()->password() );
        setScrobble( m_configWidget.data()->scrobble() );
    }

    m_infoPlugin.data()->settingsChanged();
}


QString
LastFmAccount::password() const
{
    return credentials().value( "password" ).toString();
}


void
LastFmAccount::setPassword( const QString& password )
{
    QVariantHash creds = credentials();
    creds[ "password" ] = password;
    setCredentials( creds );
}

QString
LastFmAccount::sessionKey() const
{
    return credentials().value( "sessionkey" ).toString();
}


void
LastFmAccount::setSessionKey( const QString& sessionkey )
{
    QVariantHash creds = credentials();
    creds[ "sessionkey" ] = sessionkey;
    setCredentials( creds );
}


QString
LastFmAccount::username() const
{
    return credentials().value( "username" ).toString();
}


void
LastFmAccount::setUsername( const QString& username )
{
    QVariantHash creds = credentials();
    creds[ "username" ] = username;
    setCredentials( creds );
}


bool
LastFmAccount::scrobble() const
{
    return configuration().value( "scrobble" ).toBool();
}


void
LastFmAccount::setScrobble( bool scrobble )
{
    QVariantHash conf;
    conf[ "scrobble" ] = scrobble;
    setConfiguration( conf );
}


void
LastFmAccount::resolverInstalled( const QString &resolverId )
{
    if ( resolverId == "lastfm" )
    {
        // We requested this install, so we want to launch it
        hookupResolver();
        AccountManager::instance()->enableAccount( this );
    }
}

void
LastFmAccount::resolverChanged()
{
    emit connectionStateChanged( connectionState() );
}


void
LastFmAccount::hookupResolver()
{
    // If there is a last.fm resolver from attica installed, create the corresponding ExternalResolver* and hook up to it
    const Attica::Content res = AtticaManager::instance()->resolverForId( "lastfm" );
    const AtticaManager::ResolverState state = AtticaManager::instance()->resolverState( res );
    Q_ASSERT( state == AtticaManager::Installed );

    const AtticaManager::Resolver data = AtticaManager::instance()->resolverData( res.id() );

    m_resolver = QWeakPointer< ExternalResolverGui >( qobject_cast< ExternalResolverGui* >( Pipeline::instance()->addScriptResolver( data.scriptPath, enabled() ) ) );
    connect( m_resolver.data(), SIGNAL( changed() ), this, SLOT( resolverChanged() ) );
}


Attica::Content
LastFmAccount::atticaContent() const
{
    return AtticaManager::instance()->resolverForId( "lastfm" );
}
