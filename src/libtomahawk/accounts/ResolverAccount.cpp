/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "ResolverAccount.h"

#include "AccountManager.h"
#include "AtticaManager.h"
#include "ExternalResolver.h"
#include "ExternalResolverGui.h"
#include "Pipeline.h"
#include "TomahawkSettings.h"
#include "Source.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>

using namespace Tomahawk;
using namespace Accounts;

Account*
ResolverAccountFactory::createAccount( const QString& accountId )
{
    // Can't use this to create new accounts. Needs to be able to find account in config
    // to load proper resolver account type. Creation is done from AtticaManager when path is known
    Q_ASSERT( !accountId.isEmpty() );

    // If it's an attica resolver, return it instead so we get an icon
    const bool isFromAttica = TomahawkSettings::instance()->value( QString( "accounts/%1/atticaresolver" ).arg( accountId ), false ).toBool();
    if ( isFromAttica )
        return new AtticaResolverAccount( accountId );
    else
        return new ResolverAccount( accountId );
}


Account*
ResolverAccountFactory::createFromPath( const QString& path )
{
    return createFromPath( path, factoryId(), false );
}


Account*
ResolverAccountFactory::createFromPath( const QString& path, const QString& factory,  bool isAttica )
{
    qDebug() << "Creating ResolverAccount from path:" << path << "is attica" << isAttica;
    if ( isAttica )
    {
        QFileInfo info( path );
        return new AtticaResolverAccount( generateId( factory ), path, info.baseName() );
    }
    else
        return new ResolverAccount( generateId( factory ), path );
}


ResolverAccount::ResolverAccount( const QString& accountId )
    : Account( accountId )
{
    const QString path = configuration()[ "path" ].toString();
    setTypes( AccountType( ResolverType ) );

    // We should have a valid saved path
    Q_ASSERT( !path.isEmpty() );

    init( path );
}


ResolverAccount::ResolverAccount( const QString& accountId, const QString& path )
    : Account( accountId )
{
    QVariantHash configuration;
    configuration[ "path" ] = path;
    setConfiguration( configuration );

    init( path );

    sync();
}


ResolverAccount::~ResolverAccount()
{
    if ( m_resolver.isNull() )
        return;

    Pipeline::instance()->removeScriptResolver( m_resolver.data()->filePath() );
    delete m_resolver.data();
}


void
ResolverAccount::init( const QString& path )
{
    setTypes( AccountType( ResolverType ) );

    if ( !QFile::exists( path ) )
    {
        AccountManager::instance()->disableAccount( this );
    }
    else
    {
        hookupResolver();
    }
}


void
ResolverAccount::hookupResolver()
{
    tDebug() << "Hooking up resolver:" << configuration().value( "path" ).toString() << enabled();

    m_resolver = QWeakPointer< ExternalResolverGui >( qobject_cast< ExternalResolverGui* >( Pipeline::instance()->addScriptResolver( configuration().value( "path" ).toString() ) ) );
    connect( m_resolver.data(), SIGNAL( changed() ), this, SLOT( resolverChanged() ) );

    // What resolver do we have here? Should only be types that are 'real' resolvers
    Q_ASSERT ( m_resolver.data() );

    setAccountFriendlyName( m_resolver.data()->name() );
}


void
ResolverAccount::authenticate()
{
    if ( m_resolver.isNull() )
        return;

    tDebug() << Q_FUNC_INFO << "Authenticating/starting resolver, exists?" << m_resolver.data()->name();

    if ( !m_resolver.data()->running() )
        m_resolver.data()->start();

    emit connectionStateChanged( connectionState() );
}


bool
ResolverAccount::isAuthenticated() const
{
    return !m_resolver.isNull() && m_resolver.data()->running();
}


void
ResolverAccount::deauthenticate()
{
    if ( !m_resolver.isNull() && m_resolver.data()->running() )
        m_resolver.data()->stop();

    emit connectionStateChanged( connectionState() );

}


Account::ConnectionState
ResolverAccount::connectionState() const
{
    if ( !m_resolver.isNull() && m_resolver.data()->running() )
        return Connected;
    else
        return Disconnected;
}


QWidget*
ResolverAccount::configurationWidget()
{
    if ( m_resolver.isNull() )
        return 0;

    return m_resolver.data()->configUI();
}


QString
ResolverAccount::errorMessage() const
{
    // TODO
//     return m_resolver->error();
    return QString();
}


void
ResolverAccount::removeFromConfig()
{
    // TODO
    Account::removeFromConfig();
}


void ResolverAccount::saveConfig()
{
    Account::saveConfig();
    if ( !m_resolver.isNull() )
        m_resolver.data()->saveConfig();
}


QString
ResolverAccount::path() const
{
    if ( m_resolver.isNull() )
        return QString();

    return m_resolver.data()->filePath();
}


void
ResolverAccount::resolverChanged()
{
    setAccountFriendlyName( m_resolver.data()->name() );
    emit connectionStateChanged( connectionState() );
}

QPixmap
ResolverAccount::icon() const
{
    if ( m_resolver.isNull() )
        return QPixmap();

    return m_resolver.data()->icon();
}

/// AtticaResolverAccount

AtticaResolverAccount::AtticaResolverAccount( const QString& accountId )
    : ResolverAccount( accountId )
{
    TomahawkSettings::instance()->setValue( QString( "accounts/%1/atticaresolver" ).arg( accountId ), true );

    init();
    m_atticaId = configuration().value( "atticaId" ).toString();

}

AtticaResolverAccount::AtticaResolverAccount( const QString& accountId, const QString& path, const QString& atticaId )
    : ResolverAccount( accountId, path )
    , m_atticaId( atticaId )
{
    QVariantHash conf = configuration();
    conf[ "atticaId" ] = atticaId;
    setConfiguration( conf );

    TomahawkSettings::instance()->setValue( QString( "accounts/%1/atticaresolver" ).arg( accountId ), true );

    init();
    sync();
}


AtticaResolverAccount::~AtticaResolverAccount()
{
}


void
AtticaResolverAccount::init()
{
    connect( AtticaManager::instance(), SIGNAL( resolverIconUpdated( QString ) ), this, SLOT( resolverIconUpdated( QString ) ) );

    if ( AtticaManager::instance()->resolversLoaded() )
        loadIcon();
    else
        connect( AtticaManager::instance(), SIGNAL( resolversLoaded( Attica::Content::List ) ), this, SLOT( loadIcon() ) );


}


void
AtticaResolverAccount::loadIcon()
{
    if ( m_resolver.isNull() )
        return;


    m_icon = AtticaManager::instance()->iconForResolver( AtticaManager::instance()->resolverForId( m_atticaId ) );
    m_resolver.data()->setIcon( m_icon );
}


void
AtticaResolverAccount::setPath( const QString& path )
{
    QVariantHash config = configuration();
    config[ "path" ] = path;
    setConfiguration( config );

    hookupResolver();

    sync();
}


QPixmap
AtticaResolverAccount::icon() const
{
    return m_icon;
}


void
AtticaResolverAccount::resolverIconUpdated( const QString& resolver )
{
    if ( m_atticaId == resolver )
        loadIcon();
}
