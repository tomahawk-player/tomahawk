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

#include <attica/downloaditem.h>

#include <QCoreApplication>
#include <QNetworkReply>
#include <QTemporaryFile>
#include <QDir>
#include <QTimer>

#include "utils/Logger.h"
#include "accounts/ResolverAccount.h"
#include "accounts/AccountManager.h"

using namespace Attica;

AtticaManager* AtticaManager::s_instance = 0;


class BinaryInstallerHelper : public QObject
{
    Q_OBJECT
public:
    explicit BinaryInstallerHelper( const QString& resolverId, bool createAccount, AtticaManager* manager)
        : QObject( manager )
        , m_manager( QWeakPointer< AtticaManager >( manager ) )
        , m_resolverId( resolverId )
        , m_createAccount( createAccount )
    {
        Q_ASSERT( !m_resolverId.isEmpty() );
        Q_ASSERT( !m_manager.isNull() );

        setProperty( "resolverid", m_resolverId );
    }

    virtual ~BinaryInstallerHelper() {}

public slots:
    void installSucceeded( const QString& path )
    {
        qDebug() << Q_FUNC_INFO << "install of binary resolver succeeded, enabling: " << path;

        if ( m_manager.isNull() )
            return;

        if ( m_createAccount )
        {
            Tomahawk::Accounts::Account* acct = Tomahawk::Accounts::AccountManager::instance()->accountFromPath( path );

            Tomahawk::Accounts::AccountManager::instance()->addAccount( acct );
            TomahawkSettings::instance()->addAccount( acct->accountId() );
            Tomahawk::Accounts::AccountManager::instance()->enableAccount( acct );
        }

        m_manager.data()->m_resolverStates[ m_resolverId ].scriptPath = path;
        m_manager.data()->m_resolverStates[ m_resolverId ].state = AtticaManager::Installed;

        TomahawkSettingsGui::instanceGui()->setAtticaResolverStates( m_manager.data()->m_resolverStates );
        emit m_manager.data()->resolverInstalled( m_resolverId );
        emit m_manager.data()->resolverStateChanged( m_resolverId );

        deleteLater();
    }
    void installFailed()
    {
        qDebug() << Q_FUNC_INFO << "install failed";

        if ( m_manager.isNull() )
            return;

        m_manager.data()->resolverInstallationFailed( m_resolverId );

        deleteLater();
    }

private:
    QString m_resolverId;
    bool m_createAccount;
    QWeakPointer<AtticaManager> m_manager;
};


AtticaManager::AtticaManager( QObject* parent )
    : QObject( parent )
    , m_resolverJobsLoaded( 0 )
{
    connect( &m_manager, SIGNAL( providerAdded( Attica::Provider ) ), this, SLOT( providerAdded( Attica::Provider ) ) );

    // resolvers
   m_manager.addProviderFile( QUrl( "http://bakery.tomahawk-player.org/resolvers/providers.xml" ) );
//     m_manager.addProviderFile( QUrl( "http://lycophron/resolvers/providers.xml" ) );

    qRegisterMetaType< Attica::Content >( "Attica::Content" );
}


AtticaManager::~AtticaManager()
{
    savePixmapsToCache();
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
    qDebug() << "Got custom account for?" << id << m_customAccounts.keys();
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

    syncServerData();

    if ( ++m_resolverJobsLoaded == 2 )
        emit resolversLoaded( m_resolvers );
}


void
AtticaManager::binaryResolversList( BaseJob* j )
{
    ListJob< Content >* job = static_cast< ListJob< Content >* >( j );

    Content::List binaryResolvers = job->itemList();

    // NOTE: No binary support for linux distros
    QString platform;
#ifdef Q_OS_MAC
    platform = "osx";
#elif Q_OS_WIN
    platform = "win";
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
        emit resolversLoaded( m_resolvers );
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
}


void
AtticaManager::syncServerData()
{
    // look for any newer. m_resolvers has list from server, and m_resolverStates will contain any locally installed ones
    // also update ratings
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
                    m_resolverStates[ id ].state = NeedsUpgrade;
                    QMetaObject::invokeMethod( this, "upgradeResolver", Qt::QueuedConnection, Q_ARG( Attica::Content, upstream ) );
                }
            }
        }
    }
}


void
AtticaManager::installResolver( const Content& resolver, bool autoCreateAccount )
{
    Q_ASSERT( !resolver.id().isNull() );

    if ( m_resolverStates[ resolver.id() ].state != Upgrading )
        m_resolverStates[ resolver.id() ].state = Installing;

    m_resolverStates[ resolver.id() ].scriptPath = resolver.attribute( "mainscript" );
    m_resolverStates[ resolver.id() ].version = resolver.version();
    emit resolverStateChanged( resolver.id() );

    ItemJob< DownloadItem >* job = m_resolverProvider.downloadLink( resolver.id() );
    connect( job, SIGNAL( finished( Attica::BaseJob* ) ), this, SLOT( resolverDownloadFinished( Attica::BaseJob* ) ) );
    job->setProperty( "resolverId", resolver.id() );
    job->setProperty( "createAccount", autoCreateAccount );
    job->setProperty( "binarySignature", resolver.attribute("signature"));

    job->start();
}


void
AtticaManager::upgradeResolver( const Content& resolver )
{
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
AtticaManager::resolverDownloadFinished ( BaseJob* j )
{
    ItemJob< DownloadItem >* job = static_cast< ItemJob< DownloadItem >* >( j );

    if ( job->metadata().error() == Attica::Metadata::NoError )
    {
        DownloadItem item = job->result();
        QUrl url = item.url();
        // download the resolver itself :)
        QNetworkReply* reply = TomahawkUtils::nam()->get( QNetworkRequest( url ) );
        connect( reply, SIGNAL( finished() ), this, SLOT( payloadFetched() ) );
        reply->setProperty( "resolverId", job->property( "resolverId" ) );
        reply->setProperty( "createAccount", job->property( "createAccount" ) );
        reply->setProperty( "binarySignature", job->property( "binarySignature" ) );
    }
    else
    {
        tLog() << "Failed to do resolver download job!" << job->metadata().error();
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
        QTemporaryFile f( QDir::tempPath() + QDir::separator() + "tomahawkattica_XXXXXX.zip" );
        if ( !f.open() )
        {
            tLog() << "Failed to write zip file to temp file:" << f.fileName();
            return;
        }
        f.write( reply->readAll() );
        f.close();

        if ( m_resolverStates[ resolverId ].binary )
        {
            // First ensure the signature matches. If we can't verify it, abort!
            const QString signature = reply->property( "binarySignature" ).toString();
            // Must have a signature for binary resolvers...
            Q_ASSERT( !signature.isEmpty() );
            if ( signature.isEmpty() )
                return;
            if ( !TomahawkUtils::verifyFile( f.fileName(), signature ) )
            {
                qWarning() << "FILE SIGNATURE FAILED FOR BINARY RESOLVER! WARNING! :" << f.fileName() << signature;
            }
            else
            {
                TomahawkUtils::extractBinaryResolver( f.fileName(), new BinaryInstallerHelper( resolverId, reply->property( "createAccount" ).toBool(), this ) );
                // Don't emit failed yet
                installedSuccessfully = true;
            }
        }
        else
        {
            QDir dir( TomahawkUtils::extractScriptPayload( f.fileName(), resolverId ) );
            QString resolverPath = dir.absoluteFilePath( m_resolverStates[ resolverId ].scriptPath );

            if ( !resolverPath.isEmpty() )
            {
                // update with absolute, not relative, path
                m_resolverStates[ resolverId ].scriptPath = resolverPath;

                if ( reply->property( "createAccount" ).toBool() )
                {
                    // Do the install / add to tomahawk
                    Tomahawk::Accounts::Account* resolver = Tomahawk::Accounts::ResolverAccountFactory::createFromPath( resolverPath, "resolveraccount", true );
                    Tomahawk::Accounts::AccountManager::instance()->addAccount( resolver );
                    TomahawkSettings::instance()->addAccount( resolver->accountId() );
                }

                installedSuccessfully = true;
            }
        }
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

        // remove account as well
        QList< Tomahawk::Accounts::Account* > accounts = Tomahawk::Accounts::AccountManager::instance()->accounts( Tomahawk::Accounts::ResolverType );
        foreach ( Tomahawk::Accounts::Account* account, accounts )
        {
            if ( Tomahawk::Accounts::AtticaResolverAccount* atticaAccount = qobject_cast< Tomahawk::Accounts::AtticaResolverAccount* >( account ) )
            {
                if ( atticaAccount->atticaId() == resolver.id() ) // this is the account we want to remove
                {
                    Tomahawk::Accounts::AccountManager::instance()->removeAccount( atticaAccount );
                }
            }
        }
    }

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
}

#include "AtticaManager.moc"
