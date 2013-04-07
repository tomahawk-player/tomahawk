/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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
#include "resolvers/ExternalResolver.h"
#include "resolvers/ExternalResolverGui.h"
#include "Pipeline.h"
#include "TomahawkSettings.h"
#include "Artist.h"
#include "Album.h"
#include "Source.h"
#include "utils/Logger.h"
#include "qjson/parser.h"
#include "jobview/JobStatusView.h"
#include "jobview/JobStatusModel.h"
#include "jobview/ErrorStatusMessage.h"
#include "TomahawkVersion.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>

#define MANUALRESOLVERS_DIR "manualresolvers"

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

    const QFileInfo pathInfo( path );

    if ( isAttica )
    {
        QVariantHash configuration;
        QDir dir = pathInfo.absoluteDir();//assume we are in the code directory of a bundle
        if ( dir.cdUp() && dir.cdUp() ) //go up twice to the content dir, if any
        {
            QString metadataFilePath = dir.absoluteFilePath( "metadata.json" );
            QFileInfo metadataFileInfo( metadataFilePath );
            if ( metadataFileInfo.isFile() && metadataFileInfo.isReadable() )
            {
                configuration = metadataFromJsonFile( metadataFilePath );
                expandPaths( dir, configuration );
            }
        }
        return new AtticaResolverAccount( generateId( factory ), path, pathInfo.baseName(), configuration );
    }
    else //on filesystem, but it could be a bundle or a legacy resolver file
    {
        QString realPath( path );

        QVariantHash configuration;

        if ( pathInfo.suffix() == "axe" )
        {
            QString uniqueName = uuid();
            QDir dir( TomahawkUtils::extractScriptPayload( pathInfo.filePath(),
                                                           uniqueName,
                                                           MANUALRESOLVERS_DIR ) );
            if ( !( dir.exists() && dir.isReadable() ) ) //decompression fubar
            {
                JobStatusView::instance()->model()->addJob( new ErrorStatusMessage(
                                        tr( "Resolver installation error: cannot open bundle." ) ) );
                return 0;
            }

            if ( !dir.cd( "content" ) ) //more fubar
            {
                JobStatusView::instance()->model()->addJob( new ErrorStatusMessage(
                                        tr( "Resolver installation error: incomplete bundle." ) ) );
                return 0;
            }

            QString metadataFilePath = dir.absoluteFilePath( "metadata.json" );
            configuration = metadataFromJsonFile( metadataFilePath );

            configuration[ "bundleDir" ] = uniqueName;

            if ( !configuration[ "pluginName" ].isNull() && !configuration[ "pluginName" ].toString().isEmpty() )
            {
                dir.cdUp();
                if ( !dir.cdUp() ) //we're in MANUALRESOLVERS_DIR
                    return 0;

                QString name = configuration[ "pluginName" ].toString();

                QString namePath = dir.absoluteFilePath( name );
                QFileInfo npI( namePath );

                if ( npI.exists() && npI.isDir() )
                {
                    TomahawkUtils::removeDirectory( namePath );
                }

                dir.rename( uniqueName, name );

                configuration[ "bundleDir" ] = name;

                if ( !dir.cd( QString( "%1/content" ).arg( name ) ) ) //should work if it worked once
                    return 0;
            }

            expandPaths( dir, configuration );

            realPath = configuration[ "path" ].toString();
            if ( realPath.isEmpty() )
            {
                JobStatusView::instance()->model()->addJob( new ErrorStatusMessage(
                                        tr( "Resolver installation error: bad metadata in bundle." ) ) );
                return 0;
            }
        }
        else //either legacy resolver or uncompressed bundle, so we look for a metadata file
        {
            QDir dir = pathInfo.absoluteDir();//assume we are in the code directory of a bundle
            if ( dir.cdUp() && dir.cdUp() ) //go up twice to the content dir, if any
            {
                QString metadataFilePath = dir.absoluteFilePath( "metadata.json" );
                configuration = metadataFromJsonFile( metadataFilePath );
                expandPaths( dir, configuration );
                configuration[ "path" ] = realPath; //our initial path still overrides whatever the desktop file says
            }
            //else we just have empty metadata (legacy resolver without desktop file)
        }

        //check if the bundle specifies a platform, and if so, reject the resolver if the platform is wrong
        if ( !configuration[ "platform" ].isNull() && configuration[ "platform" ].toString() != "any" )
        {
            QString platform( configuration[ "platform" ].toString() );
            QString myPlatform( "any" );

#if defined( Q_OS_WIN )
            myPlatform = "win";
#elif defined( Q_OS_MAC )
            myPlatform = "osx";
#elif defined( Q_OS_LINUX )
            if ( __WORDSIZE == 32 )
                myPlatform = "linux-x86";
            else if ( __WORDSIZE == 64 )
                myPlatform = "linux-x64";
#endif

            if ( !myPlatform.contains( platform ) )
            {
                tDebug() << "Wrong resolver platform.";
                JobStatusView::instance()->model()->addJob( new ErrorStatusMessage(
                                        tr( "Resolver installation error: platform mismatch." ) ) );
                return 0;
            }
        }

        if ( !configuration[ "tomahawkVersion" ].isNull() )
        {
            QString thVer = TOMAHAWK_VERSION;
            QString requiredVer = configuration[ "tomahawkVersion" ].toString();

            if ( TomahawkUtils::compareVersionStrings( thVer, requiredVer ) < 0 )
            {
                JobStatusView::instance()->model()->addJob( new ErrorStatusMessage(
                                        tr( "Resolver installation error: Tomahawk %1 or newer is required." )
                                        .arg( requiredVer ) ) );
                return 0;
            }
        }

        //TODO: handle multi-account resolvers

        return new ResolverAccount( generateId( factory ), realPath, configuration );
    }
}


QVariantHash
ResolverAccountFactory::metadataFromJsonFile( const QString& path )
{
    QVariantHash result;
    QFile metadataFile( path );
    if ( metadataFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        QJson::Parser parser;
        bool ok;
        QVariantMap variant = parser.parse( metadataFile.readAll(), &ok ).toMap();

        if ( ok )
        {
            result[ "pluginName" ] = variant[ "pluginName" ];
            result[ "author" ] = variant[ "author" ];
            result[ "description" ] = variant[ "description" ];
            if ( !variant[ "manifest" ].isNull() )
            {
                QVariantMap manifest = variant[ "manifest" ].toMap();
                if ( !manifest[ "main" ].isNull() )
                {
                    result[ "path" ] = manifest[ "main" ]; //this is our path to the main JS script
                }
                if ( !manifest[ "scripts" ].isNull() )
                {
                    result[ "scripts" ] = manifest[ "scripts" ]; //any additional scripts to load before
                }
            }
            if ( !variant[ "version" ].isNull() )
                result[ "version" ] = variant[ "version" ];
            if ( !variant[ "revision" ].isNull() )
                result[ "revision" ] = variant[ "revision" ];
            if ( !variant[ "timestamp" ].isNull() )
                result[ "timestamp" ] = variant[ "timestamp" ];
            if ( !variant[ "tomahawkVersion" ].isNull() )
                result[ "tomahawkVersion" ] = variant[ "tomahawkVersion" ];
            if ( !variant[ "platform" ].isNull() )
                result[ "platform" ] = variant[ "platform" ];
        }
    }
    return result;
}


void
ResolverAccountFactory::expandPaths( const QDir& contentDir, QVariantHash& configuration )
{
    if ( !configuration[ "path" ].isNull() )
    {
        configuration[ "path" ] = contentDir.absoluteFilePath( configuration[ "path" ].toString() ); //this is our path to the JS
    }
    if ( !configuration[ "scripts" ].isNull() )
    {
        QStringList scripts;
        foreach ( QString s, configuration[ "scripts" ].toStringList() )
        {
            scripts << contentDir.absoluteFilePath( s );
        }
        configuration[ "scripts" ] = scripts;
    }
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


ResolverAccount::ResolverAccount( const QString& accountId, const QString& path, const QVariantHash& initialConfiguration )
    : Account( accountId )
{
    QVariantHash configuration( initialConfiguration );
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

    QString mainScriptPath = configuration().value( "path" ).toString();
    QStringList additionalPaths;
    if ( configuration().contains( "scripts" ) )
        additionalPaths = configuration().value( "scripts" ).toStringList();

    Tomahawk::ExternalResolver* er = Pipeline::instance()->addScriptResolver( mainScriptPath, additionalPaths );
    m_resolver = QPointer< ExternalResolverGui >( qobject_cast< ExternalResolverGui* >( er ) );
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


AccountConfigWidget*
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


void
ResolverAccount::saveConfig()
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


QString
ResolverAccount::description() const
{
    return configuration().value( "description" ).toString();
}


QString
ResolverAccount::author() const
{
    return configuration().value( "author" ).toString();
}


QString
ResolverAccount::version() const
{
    QString versionString = configuration().value( "version" ).toString();
    QString build = configuration().value( "revision" ).toString();
    if ( !build.isEmpty() )
        return versionString + "-" + build;
    return versionString;
}


void
ResolverAccount::removeBundle()
{
    QString bundleDir = configuration()[ "bundleDir" ].toString();
    if ( bundleDir.isEmpty() )
        return;

    QString expectedPath = TomahawkUtils::appDataDir().absoluteFilePath( QString( "%1/%2" ).arg( MANUALRESOLVERS_DIR ).arg( bundleDir ) );
    QFileInfo fi( expectedPath );
    if ( fi.exists() && fi.isDir() && fi.isWritable() )
    {
        TomahawkUtils::removeDirectory( expectedPath );
    }
}


/// AtticaResolverAccount

AtticaResolverAccount::AtticaResolverAccount( const QString& accountId )
    : ResolverAccount( accountId )
{
    TomahawkSettings::instance()->setValue( QString( "accounts/%1/atticaresolver" ).arg( accountId ), true );

    init();
    m_atticaId = configuration().value( "atticaId" ).toString();

}

AtticaResolverAccount::AtticaResolverAccount( const QString& accountId, const QString& path, const QString& atticaId, const QVariantHash& initialConfiguration )
    : ResolverAccount( accountId, path, initialConfiguration )
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
