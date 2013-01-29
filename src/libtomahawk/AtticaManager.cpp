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

#include "AtticaManager.h"

#include "utils/TomahawkUtils.h"
#include "TomahawkSettingsGui.h"
#include "Pipeline.h"
#include "Source.h"
#include "config.h"

#include <attica/downloaditem.h>

#include <QCoreApplication>
#include <QNetworkReply>
#include <QTemporaryFile>
#include <QDir>
#include <QTimer>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>

#include "utils/Logger.h"
#include "accounts/ResolverAccount.h"
#include "accounts/AccountManager.h"
#include "utils/BinaryInstallerHelper.h"
#include "utils/Closure.h"

using namespace Attica;

AtticaManager* AtticaManager::s_instance = 0;


// Sort binary resolvers above script resolvers, and script resolvers by download count
bool
resolverSort( const Attica::Content& first, const Attica::Content& second )
{
    if ( !first.attribute( "typeid" ).isEmpty() && second.attribute( "typeid" ).isEmpty() )
        return true;

    return first.downloads() > second.downloads();
}


AtticaManager::AtticaManager( QObject* parent )
    : QObject( parent )
    , m_manager( Attica::ProviderManager::ProviderFlags( Attica::ProviderManager::DisablePlugins ) )
    , m_resolverJobsLoaded( 0 )
{
    connect( &m_manager, SIGNAL( providerAdded( Attica::Provider ) ), this, SLOT( providerAdded( Attica::Provider ) ) );

    // resolvers
//    m_manager.addProviderFile( QUrl( "http://bakery.tomahawk-player.org/resolvers/providers.xml" ) );

    const QString url = QString( "%1/resolvers/providers.xml?version=%2" ).arg( hostname() ).arg( TomahawkUtils::appFriendlyVersion() );
    QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( QUrl( url ) ) );
    NewClosure( reply, SIGNAL( finished() ), this, SLOT( providerFetched( QNetworkReply* ) ), reply );
    connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ), this, SLOT( providerError( QNetworkReply::NetworkError ) ) );

//     m_manager.addProviderFile( QUrl( "http://lycophron/resolvers/providers.xml" ) );

    qRegisterMetaType< Attica::Content >( "Attica::Content" );
}


AtticaManager::~AtticaManager()
{
    savePixmapsToCache();


    foreach( const QString& id, m_resolverStates.keys() )
    {
        if ( !m_resolverStates[ id ].pixmap )
            continue;

        delete m_resolverStates[ id ].pixmap;
    }
}


void
AtticaManager::fetchMissingIcons()
{
    foreach ( Content resolver, m_resolvers )
    {
        if ( !m_resolverStates.contains( resolver.id() ) )
            m_resolverStates.insert( resolver.id(), Resolver() );

        if ( !m_resolverStates.value( resolver.id() ).pixmap && !resolver.icons().isEmpty() && !resolver.icons().first().url().isEmpty() )
        {
            QNetworkReply* fetch = TomahawkUtils::nam()->get( QNetworkRequest( resolver.icons().first().url() ) );
            fetch->setProperty( "resolverId", resolver.id() );

            connect( fetch, SIGNAL( finished() ), this, SLOT( resolverIconFetched() ) );
        }
    }
}


QString
AtticaManager::hostname() const
{
    return "http://bakery.tomahawk-player.org";
}

void
AtticaManager::loadPixmapsFromCache()
{
    QDir cacheDir = TomahawkUtils::appDataDir();
    if ( !cacheDir.cd( "atticacache" ) ) // doesn't exist, no cache
        return;

    qDebug() << "Loading resolvers from cache dir:" << cacheDir.absolutePath();
    qDebug() << "Currently we know about these resolvers:" << m_resolverStates.keys();
    foreach ( const QString& file, cacheDir.entryList( QStringList() << "*.png", QDir::Files | QDir::NoSymLinks ) )
    {
        // load all the pixmaps
        QFileInfo info( file );
        if ( !m_resolverStates.contains( info.baseName() ) )
        {
            tLog() << "Found resolver icon cached for resolver we no longer see in synchrotron repo:" << info.baseName();
            continue;
        }

        QPixmap* icon = new QPixmap( cacheDir.absoluteFilePath( file ) );
        if ( !icon->isNull() )
        {
            m_resolverStates[ info.baseName() ].pixmap = icon;
        }
    }
}


void
AtticaManager::savePixmapsToCache()
{
    QDir cacheDir = TomahawkUtils::appDataDir();
    if ( !cacheDir.cd( "atticacache" ) ) // doesn't exist, create
    {
        cacheDir.mkdir( "atticacache" );
        cacheDir.cd( "atticache" );
    }

    foreach( const QString& id, m_resolverStates.keys() )
    {
        if ( !m_resolverStates[ id ].pixmap || !m_resolverStates[ id ].pixmapDirty )
            continue;

        const QString filename = cacheDir.absoluteFilePath( QString( "%1.png" ).arg( id ) );
        QFile f( filename );
        if ( !f.open( QIODevice::WriteOnly ) )
        {
            tLog() << "Failed to open cache file for writing:" << filename;
        }
        else
        {
            if ( !m_resolverStates[ id ].pixmap->save( &f ) )
            {
                tLog() << "Failed to save pixmap into opened file for writing:" << filename;
            }
        }
    }
}


QPixmap
AtticaManager::iconForResolver( const Content& resolver )
{
    if ( !m_resolverStates[ resolver.id() ].pixmap )
        return QPixmap();

    return *m_resolverStates.value( resolver.id() ).pixmap;
}


Content::List
AtticaManager::resolvers() const
{
    return m_resolvers;
}


Content
AtticaManager::resolverForId( const QString& id ) const
{
    foreach ( const Attica::Content& c, m_resolvers )
    {
        if ( c.id() == id )
            return c;
    }

    return Content();
}



AtticaManager::ResolverState
AtticaManager::resolverState ( const Content& resolver ) const
{
    if ( !m_resolverStates.contains( resolver.id() ) )
    {
        return AtticaManager::Uninstalled;
    }

    return m_resolverStates[ resolver.id() ].state;
}


bool
AtticaManager::resolversLoaded() const
{
    return !m_resolvers.isEmpty();
}


QString
AtticaManager::pathFromId( const QString& resolverId ) const
{
    if ( !m_resolverStates.contains( resolverId ) )
        return QString();

    return m_resolverStates.value( resolverId ).scriptPath;
}


void
AtticaManager::uploadRating( const Content& c )
{
    m_resolverStates[ c.id() ].userRating = c.rating();

    for ( int i = 0; i < m_resolvers.count(); i++ )
    {
        if ( m_resolvers[ i ].id() == c.id() )
        {
            Attica::Content atticaContent = m_resolvers[ i ];
            atticaContent.setRating( c.rating() );
            m_resolvers[ i ] = atticaContent;
            break;
        }
    }

    TomahawkSettingsGui::instanceGui()->setAtticaResolverStates( m_resolverStates );

    PostJob* job = m_resolverProvider.voteForContent( c.id(), (uint)c.rating() );
    connect( job, SIGNAL( finished( Attica::BaseJob* ) ), job, SLOT( deleteLater() ) );

    job->start();

    emit resolverStateChanged( c.id() );
}


bool
AtticaManager::userHasRated( const Content& c ) const
{
    return m_resolverStates[ c.id() ].userRating != -1;
}


bool
AtticaManager::hasCustomAccountForAttica( const QString &id ) const
{
    return m_customAccounts.keys().contains( id );
}


Tomahawk::Accounts::Account*
AtticaManager::customAccountForAttica( const QString &id ) const
{
    return m_customAccounts.value( id );
}


void
AtticaManager::registerCustomAccount( const QString &atticaId, Tomahawk::Accounts::Account *account )
{
    m_customAccounts.insert( atticaId, account );
}


AtticaManager::Resolver
AtticaManager::resolverData(const QString &atticaId) const
{
    return m_resolverStates.value( atticaId );
}


void
AtticaManager::providerError( QNetworkReply::NetworkError err )
{
    Q_UNUSED( err );

    // So those who care know
    emit resolversLoaded( Content::List() );
}


void
AtticaManager::providerFetched( QNetworkReply* reply )
{
    Q_ASSERT( reply );
    if ( !reply )
        return;

    m_manager.addProviderFromXml( reply->readAll() );
}


void
AtticaManager::providerAdded( const Provider& provider )
{
    if ( provider.name() == "Tomahawk Resolvers" )
    {
        m_resolverProvider = provider;
        m_resolvers.clear();

        m_resolverStates = TomahawkSettingsGui::instanceGui()->atticaResolverStates();

        ListJob<Category>* job = m_resolverProvider.requestCategories();
        connect( job, SIGNAL( finished( Attica::BaseJob* ) ), this, SLOT( categoriesReturned( Attica::BaseJob* ) ) );
        job->start();
    }
}


void
AtticaManager::categoriesReturned( BaseJob* j )
{
    ListJob< Category >* job = static_cast< ListJob< Category >* >( j );

    Category::List categories = job->itemList();
    foreach ( const Category& category, categories )
    {
        ListJob< Content >* job = m_resolverProvider.searchContents( Category::List() << category, QString(), Provider::Downloads, 0, 50 );

        if ( category.name() == "Resolver" )
            connect( job, SIGNAL( finished( Attica::BaseJob* ) ), this, SLOT( resolversList( Attica::BaseJob* ) ) );
        else if ( category.name() == "BinaryResolver" )
            connect( job, SIGNAL( finished( Attica::BaseJob* ) ), this, SLOT( binaryResolversList( Attica::BaseJob* ) ) );

        job->start();
    }
}


void
AtticaManager::resolversList( BaseJob* j )
{
    ListJob< Content >* job = static_cast< ListJob< Content >* >( j );

    m_resolvers.append( job->itemList() );

    // Sanity check. if any resolvers are installed that don't exist on the hd, remove them.
    foreach ( const QString& rId, m_resolverStates.keys() )
    {
        if ( m_resolverStates[ rId ].state == Installed ||
             m_resolverStates[ rId ].state == NeedsUpgrade )
        {
            if ( m_resolverStates[ rId ].binary )
                continue;

            // Guess location on disk
            QDir dir( QString( "%1/atticaresolvers/%2" ).arg( TomahawkUtils::appDataDir().absolutePath() ).arg( rId ) );
            if ( !dir.exists() )
            {
                // Uh oh
                qWarning() << "Found attica resolver marked as installed that didn't exist on disk! Setting to uninstalled: " << rId << dir.absolutePath();
                m_resolverStates[ rId ].state = Uninstalled;
                TomahawkSettingsGui::instanceGui()->setAtticaResolverState( rId, Uninstalled );
            }
        }
    }

    // load icon cache from disk, and fetch any we are missing
    loadPixmapsFromCache();

    fetchMissingIcons();


    if ( ++m_resolverJobsLoaded == 2 )
    {
        qSort( m_resolvers.begin(), m_resolvers.end(), resolverSort );
        syncServerData();
        emit resolversLoaded( m_resolvers );
    }
}


void
AtticaManager::binaryResolversList( BaseJob* j )
{
    ListJob< Content >* job = static_cast< ListJob< Content >* >( j );

    Content::List binaryResolvers = job->itemList();

    QString platform;
#if defined(Q_OS_MAC)
    platform = "osx";
#elif defined(Q_OS_WIN)
    platform = "win";
#elif defined(Q_OS_LINUX) && defined(__GNUC__) && defined(__x86_64__)
    platform = "linux-x64";
#elif defined(Q_OS_LINUX) // Horrible assumption here...
    platform = "linux-x86";
#endif

    // Override if no binary resolvers were requested
#ifndef WITH_BINARY_ATTICA
    platform = QString();
#endif

    foreach ( const Content& c, binaryResolvers )
    {
        if ( !c.attribute( "typeid" ).isEmpty() && c.attribute( "typeid" ) == platform )
        {
            // We have a binary resolver for this platform
            qDebug() << "WE GOT A BINARY RESOLVER:" << c.id() << c.name() << c.attribute( "signature" );
            m_resolvers.append( c );
            if ( !m_resolverStates.contains( c.id() ) )
            {
                Resolver r;
                r.binary = true;
                m_resolverStates.insert( c.id(), r );
            }
            else if ( m_resolverStates[ c.id() ].binary != true )
            { // HACK workaround... why is this not set in the first place sometimes? Migration issue?
                m_resolverStates[ c.id() ].binary = true;
            }


        }
    }

    if ( ++m_resolverJobsLoaded == 2 )
    {
        qSort( m_resolvers.begin(), m_resolvers.end(), resolverSort );
        syncServerData();
        emit resolversLoaded( m_resolvers );
    }
}


void
AtticaManager::resolverIconFetched()
{
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( reply );

    const QString resolverId = reply->property( "resolverId" ).toString();

    if ( !reply->error() == QNetworkReply::NoError )
    {
        tLog() << "Failed to fetch resolver icon image:" << reply->errorString();
        return;
    }

    QByteArray data = reply->readAll();
    QPixmap* icon = new QPixmap;
    icon->loadFromData( data );
    m_resolverStates[ resolverId ].pixmap = icon;
    m_resolverStates[ resolverId ].pixmapDirty = true;

    emit resolverIconUpdated( resolverId );
}


void
AtticaManager::syncServerData()
{
    // look for any newer. m_resolvers has list from server, and m_resolverStates will contain any locally installed ones
    // also update ratings
    tLog() << "Syncing server data!";
    foreach ( const QString& id, m_resolverStates.keys() )
    {
        Resolver r = m_resolverStates[ id ];
        for ( int i = 0; i < m_resolvers.size(); i++ )
        {
            Attica::Content upstream = m_resolvers[ i ];
            // same resolver
            if ( id != upstream.id() )
                continue;

            // Update our rating with the server's idea of rating if we haven't rated it
            if ( m_resolverStates[ id ].userRating != -1 )
            {
                upstream.setRating( m_resolverStates[ id ].userRating );
                m_resolvers[ i ] = upstream;
            }

            // DO we need to upgrade?
            if ( ( r.state == Installed || r.state == NeedsUpgrade ) &&
                 !upstream.version().isEmpty() )
            {
                if ( TomahawkUtils::newerVersion( r.version, upstream.version() ) )
                {
                    tLog() << "Doing upgrade of: " << id;
                    m_resolverStates[ id ].state = NeedsUpgrade;
                    QMetaObject::invokeMethod( this, "upgradeResolver", Qt::QueuedConnection, Q_ARG( Attica::Content, upstream ) );
                }
            }
        }
    }
}


void
AtticaManager::installResolver( const Content& resolver, bool autoCreate )
{
    doInstallResolver( resolver, autoCreate, 0 );
}


void
AtticaManager::installResolverWithHandler( const Content& resolver, Tomahawk::Accounts::AtticaResolverAccount* handler )
{
    doInstallResolver( resolver, false, handler );
}


void AtticaManager::doInstallResolver( const Content& resolver, bool autoCreate, Tomahawk::Accounts::AtticaResolverAccount* handler )
{
    Q_ASSERT( !resolver.id().isNull() );

    emit startedInstalling( resolver.id() );

    if ( m_resolverStates[ resolver.id() ].state != Upgrading )
        m_resolverStates[ resolver.id() ].state = Installing;

    m_resolverStates[ resolver.id() ].scriptPath = resolver.attribute( "mainscript" );
    m_resolverStates[ resolver.id() ].version = resolver.version();
    emit resolverStateChanged( resolver.id() );

//    ItemJob< DownloadItem >* job = m_resolverProvider.downloadLink( resolver.id() );
    QUrl url( QString( "%1/resolvers/v1/content/download/%2/1" ).arg( hostname() ).arg( resolver.id() ) );
    url.addQueryItem( "tomahawkversion", TomahawkUtils::appFriendlyVersion() );
    QNetworkReply* r = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
    NewClosure( r, SIGNAL( finished() ), this, SLOT( resolverDownloadFinished( QNetworkReply* ) ), r );
    r->setProperty( "resolverId", resolver.id() );
    r->setProperty( "createAccount", autoCreate );
    r->setProperty( "handler", QVariant::fromValue< QObject* >( handler ) );
    r->setProperty( "binarySignature", resolver.attribute("signature"));
}


void
AtticaManager::upgradeResolver( const Content& resolver )
{
    tLog() << "UPGRADING:" << resolver.id() << m_resolverStates[ resolver.id() ].state;
    Q_ASSERT( m_resolverStates.contains( resolver.id() ) );
    Q_ASSERT( m_resolverStates[ resolver.id() ].state == NeedsUpgrade );

    if ( !m_resolverStates.contains( resolver.id() ) || m_resolverStates[ resolver.id() ].state != NeedsUpgrade )
        return;

    m_resolverStates[ resolver.id() ].state = Upgrading;
    emit resolverStateChanged( resolver.id() );

    uninstallResolver( resolver );
    installResolver( resolver, false );
}


void
AtticaManager::resolverDownloadFinished ( QNetworkReply *j )
{
    Q_ASSERT( j );
    if ( !j )
        return;

    if ( j->error() == QNetworkReply::NoError )
    {
        QDomDocument doc;
        doc.setContent( j );

       const QDomNodeList nodes = doc.documentElement().elementsByTagName( "downloadlink" );
       if ( nodes.length() < 1 )
       {
           tLog() << "Found no download link for resolver:" << doc.toString();
           return;
       }

       QUrl url( nodes.item( 0 ).toElement().text() );
       // download the resolver itself :)
       tDebug() << "Downloading resolver from url:" << url.toString();

       const QDomNodeList signatures = doc.documentElement().elementsByTagName( "signature" );

       // Use the original signature provided
       QString signature = j->property( "binarySignature" ).toString();
       if ( signatures.size() > 0 )
       {
            // THis download has an overriding signature. Take that one instead
           const QString sig = signatures.item( 0 ).toElement().text();
           tLog() << "Found overridden signature in binary download:" << sig;
           signature = sig;
       }
       QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
       connect( reply, SIGNAL( finished() ), this, SLOT( payloadFetched() ) );
       reply->setProperty( "resolverId", j->property( "resolverId" ) );
       reply->setProperty( "createAccount", j->property( "createAccount" ) );
       reply->setProperty( "handler", j->property( "handler" ) );
       reply->setProperty( "binarySignature", signature );
    }
    else
    {
        tLog() << "Failed to do resolver download job!" << j->errorString() << j->error();
    }
}


void
AtticaManager::payloadFetched()
{
    QNetworkReply* reply = qobject_cast< QNetworkReply* >( sender() );
    Q_ASSERT( reply );

    bool installedSuccessfully = false;
    const QString resolverId = reply->property( "resolverId" ).toString();

    // we got a zip file, save it to a temporary file, then unzip it to our destination data dir
    if ( reply->error() == QNetworkReply::NoError )
    {
        QTemporaryFile* f = new QTemporaryFile( QDir::tempPath() + QDir::separator() + "tomahawkattica_XXXXXX.zip" );
        if ( !f->open() )
        {
            tLog() << "Failed to write zip file to temp file:" << f->fileName();
            return;
        }
        f->write( reply->readAll() );
        f->close();

        if ( m_resolverStates[ resolverId ].binary )
        {
            // First ensure the signature matches. If we can't verify it, abort!
            const QString signature = reply->property( "binarySignature" ).toString();
            // Must have a signature for binary resolvers...
            Q_ASSERT( !signature.isEmpty() );
            if ( signature.isEmpty() )
                return;
            if ( !TomahawkUtils::verifyFile( f->fileName(), signature ) )
            {
                qWarning() << "FILE SIGNATURE FAILED FOR BINARY RESOLVER! WARNING! :" << f->fileName() << signature;
            }
            else
            {
                TomahawkUtils::extractBinaryResolver( f->fileName(), new BinaryInstallerHelper( f, resolverId, reply->property( "createAccount" ).toBool(), this ) );
                // Don't emit success or failed yet, helper will do that.
                return;
            }
        }
        else
        {
            QDir dir( TomahawkUtils::extractScriptPayload( f->fileName(), resolverId ) );
            QString resolverPath = dir.absoluteFilePath( m_resolverStates[ resolverId ].scriptPath );

            if ( !resolverPath.isEmpty() )
            {
                // update with absolute, not relative, path
                m_resolverStates[ resolverId ].scriptPath = resolverPath;

                Tomahawk::Accounts::AtticaResolverAccount* handlerAccount = qobject_cast< Tomahawk::Accounts::AtticaResolverAccount* >( reply->property( "handler" ).value< QObject* >() );
                const bool createAccount = reply->property( "createAccount" ).toBool();
                if ( handlerAccount )
                {
                    handlerAccount->setPath( resolverPath );
                    Tomahawk::Accounts::AccountManager::instance()->enableAccount( handlerAccount );
                }
                else if ( createAccount )
                {
                    // Do the install / add to tomahawk
                    Tomahawk::Accounts::Account* resolver = Tomahawk::Accounts::ResolverAccountFactory::createFromPath( resolverPath, "resolveraccount", true );
                    Tomahawk::Accounts::AccountManager::instance()->addAccount( resolver );
                    TomahawkSettings::instance()->addAccount( resolver->accountId() );
                }

                fetchMissingIcons();
                installedSuccessfully = true;
            }
        }

        delete f;
    }
    else
    {
        tLog() << "Failed to download attica payload...:" << reply->errorString();
    }


    if ( installedSuccessfully )
    {
        m_resolverStates[ resolverId ].state = Installed;
        TomahawkSettingsGui::instanceGui()->setAtticaResolverStates( m_resolverStates );
        emit resolverInstalled( resolverId );
        emit resolverStateChanged( resolverId );
    }
    else
    {
        emit resolverInstallationFailed( resolverId );
    }
}


void
AtticaManager::uninstallResolver( const QString& pathToResolver )
{
    // when is this used? find and fix
    Q_ASSERT(false);

    // User manually removed a resolver not through attica dialog, simple remove
    QRegExp r( ".*([^/]*)/contents/code/main.js" );
    r.indexIn( pathToResolver );
    const QString& atticaId = r.cap( 1 );
    tDebug() << "Got resolver ID to remove:" << atticaId;
    if ( !atticaId.isEmpty() ) // this is an attica-installed resolver, mark as uninstalled
    {
        foreach ( const Content& resolver, m_resolvers )
        {
            if ( resolver.id() == atticaId ) // this is the one
            {
                m_resolverStates[ atticaId ].state = Uninstalled;
                delete m_resolverStates[ resolver.id() ].pixmap;
                m_resolverStates[ atticaId ].pixmap = 0;
                TomahawkSettingsGui::instanceGui()->setAtticaResolverState( atticaId, Uninstalled );

                doResolverRemove( atticaId );
            }
        }
    }
}


void
AtticaManager::uninstallResolver( const Content& resolver )
{
    if ( m_resolverStates[ resolver.id() ].state != Upgrading )
    {
        emit resolverUninstalled( resolver.id() );
        emit resolverStateChanged( resolver.id() );

        m_resolverStates[ resolver.id() ].state = Uninstalled;
        TomahawkSettingsGui::instanceGui()->setAtticaResolverState( resolver.id(), Uninstalled );
    }

    delete m_resolverStates[ resolver.id() ].pixmap;
    m_resolverStates[ resolver.id() ].pixmap = 0;

    doResolverRemove( resolver.id() );
}


void
AtticaManager::doResolverRemove( const QString& id ) const
{
    // uninstalling is easy... just delete it! :)
    QDir resolverDir = TomahawkUtils::appDataDir();
    if ( !resolverDir.cd( QString( "atticaresolvers/%1" ).arg( id ) ) )
        return;

    if ( id.isEmpty() )
        return;

    // sanity check
    if ( !resolverDir.absolutePath().contains( "atticaresolvers" ) ||
        !resolverDir.absolutePath().contains( id ) )
        return;

    TomahawkUtils::removeDirectory( resolverDir.absolutePath() );

    QDir cacheDir = TomahawkUtils::appDataDir();
    if ( !cacheDir.cd( "atticacache" ) )
        return;

    const bool removed = cacheDir.remove( id + ".png" );
    tDebug() << "Tried to remove cached image, succeeded?" << removed << cacheDir.filePath( id );
}

