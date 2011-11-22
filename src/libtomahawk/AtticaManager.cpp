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

#include "utils/tomahawkutils.h"
#include "tomahawksettings.h"
#include "pipeline.h"

#include <attica/downloaditem.h>
#include <quazip.h>
#include <quazipfile.h>

#include <QNetworkReply>
#include <QTemporaryFile>
#include <QDir>
#include <QTimer>

#include "utils/logger.h"

using namespace Attica;

AtticaManager* AtticaManager::s_instance = 0;


AtticaManager::AtticaManager( QObject* parent )
    : QObject( parent )
{
    connect( &m_manager, SIGNAL( providerAdded( Attica::Provider ) ), this, SLOT( providerAdded( Attica::Provider ) ) );

    // resolvers
    m_manager.addProviderFile( QUrl( "http://bakery.tomahawk-player.org:10480/resolvers/providers.xml" ) );

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
        m_resolverStates[ info.baseName() ].pixmap = icon;
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
        if ( !m_resolverStates[ id ].pixmap )
            continue;

        const QString filename = cacheDir.absoluteFilePath( QString( "%1.png" ).arg( id ) );
        if ( !m_resolverStates[ id ].pixmap->save( filename ) )
        {
            tLog() << "Failed to open cache file for writing:" << filename;
            continue;
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

    TomahawkSettings::instance()->setAtticaResolverStates( m_resolverStates );

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


void
AtticaManager::providerAdded( const Provider& provider )
{
    if ( provider.name() == "Tomahawk Resolvers" )
    {
        m_resolverProvider = provider;

        ListJob< Content >* job = m_resolverProvider.searchContents( Category::List(), QString(), Provider::Downloads, 0, 30 );
        connect( job, SIGNAL( finished( Attica::BaseJob* ) ), this, SLOT( resolversList( Attica::BaseJob* ) ) );
        job->start();
    }
}


void
AtticaManager::resolversList( BaseJob* j )
{
    ListJob< Content >* job = static_cast< ListJob< Content >* >( j );

    m_resolvers = job->itemList();
    m_resolverStates = TomahawkSettings::instance()->atticaResolverStates();

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
                if ( newerVersion( r.version, upstream.version() ) )
                {
                    m_resolverStates[ id ].state = NeedsUpgrade;
                    QMetaObject::invokeMethod( this, "upgradeResolver", Qt::QueuedConnection, Q_ARG( Attica::Content, upstream ) );
                }
            }
        }
    }
}

bool
AtticaManager::newerVersion( const QString& older, const QString& newer ) const
{
    // Version comparison: turns X.Y.Z into XYZ (adding 0 to X.Y. and 00 to X respectively)
    if ( older.isEmpty() || newer.isEmpty() )
        return false;

    QString oldDigits = older;
    oldDigits = oldDigits.replace( ".", "" );
    int oldDigitsInt;

    if ( oldDigits.size() == 1 )
    {
        oldDigitsInt = oldDigits.append("00").toInt();
    }
    else if ( oldDigits.size() == 2 )
    {
        oldDigitsInt = oldDigits.append("0").toInt();
    }
    else if ( oldDigits.size() == 3 )
    {
        oldDigitsInt = oldDigits.toInt();
    }
    else
        return false;
    
    QString newDigits = newer;
    newDigits = newDigits.replace( ".", "");
    int newDigitsInt;

    if ( newDigits.size() == 1 )
    {
        newDigitsInt = newDigits.append("00").toInt();
    }
    else if ( newDigits.size() == 2 )
    {
        newDigitsInt = newDigits.append("0").toInt();
    }
    else if ( newDigits.size() == 3 )
    {
        newDigitsInt = newDigits.toInt();    
    }
    else
        return false;

    // Do the comparison
    if ( newDigitsInt > oldDigitsInt )
        return true;

    return false;
}

void
AtticaManager::installResolver( const Content& resolver )
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
    installResolver( resolver );
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

        QString resolverId = reply->property( "resolverId" ).toString();
        QDir dir( extractPayload( f.fileName(), resolverId ) );
        QString resolverPath = dir.absoluteFilePath( m_resolverStates[ resolverId ].scriptPath );

        if ( !resolverPath.isEmpty() )
        {
            // update with absolute, not relative, path
            m_resolverStates[ resolverId ].scriptPath = resolverPath;

            // Do the install / add to tomahawk
            Tomahawk::Pipeline::instance()->addScriptResolver( resolverPath, true );
            m_resolverStates[ resolverId ].state = Installed;
            TomahawkSettings::instance()->setAtticaResolverStates( m_resolverStates );
            emit resolverInstalled( resolverId );
            emit resolverStateChanged( resolverId );
        }
    }
    else
    {
        tLog() << "Failed to download attica payload...:" << reply->errorString();
    }
}


QString
AtticaManager::extractPayload( const QString& filename, const QString& resolverId ) const
{
    // uses QuaZip to extract the temporary zip file to the user's tomahawk data/resolvers directory
    QuaZip zipFile( filename );
    if ( !zipFile.open( QuaZip::mdUnzip ) )
    {
        tLog() << "Failed to QuaZip open:" << zipFile.getZipError();
        return QString();
    }

    if ( !zipFile.goToFirstFile() )
    {
        tLog() << "Failed to go to first file in zip archive: " << zipFile.getZipError();
        return QString();
    }

    QDir resolverDir = TomahawkUtils::appDataDir();
    if ( !resolverDir.mkpath( QString( "atticaresolvers/%1" ).arg( resolverId ) ) )
    {
        tLog() << "Failed to mkdir resolver save dir: " << TomahawkUtils::appDataDir().absoluteFilePath( QString( "atticaresolvers/%1" ).arg( resolverId ) );
        return QString();
    }
    resolverDir.cd( QString( "atticaresolvers/%1" ).arg( resolverId ) );
    tDebug() << "Installing resolver to:" << resolverDir.absolutePath();

    QuaZipFile fileInZip( &zipFile );
    do
    {
        QuaZipFileInfo info;
        zipFile.getCurrentFileInfo( &info );

        if ( !fileInZip.open( QIODevice::ReadOnly ) )
        {
            tLog() << "Failed to open file inside zip archive:" << info.name << zipFile.getZipName() << "with error:" << zipFile.getZipError();
            continue;
        }

        QFile out( resolverDir.absoluteFilePath( fileInZip.getActualFileName() ) );

        QStringList parts = fileInZip.getActualFileName().split( "/" );
        if ( parts.size() > 1 )
        {
            QStringList dirs = parts.mid( 0, parts.size() - 1 );
            QString dirPath = dirs.join( "/" ); // QDir translates / to \ internally if necessary
            resolverDir.mkpath( dirPath );
        }

        // make dir if there is one needed
        QDir d( fileInZip.getActualFileName() );

        tDebug() << "Writing to output file..." << out.fileName();
        if ( !out.open( QIODevice::WriteOnly ) )
        {
            tLog() << "Failed to open resolver extract file:" << out.errorString() << info.name;
            continue;
        }


        out.write( fileInZip.readAll() );
        out.close();
        fileInZip.close();

    } while ( zipFile.goToNextFile() );

    return resolverDir.absolutePath();
}


void
AtticaManager::uninstallResolver( const QString& pathToResolver )
{
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
                TomahawkSettings::instance()->setAtticaResolverState( atticaId, Uninstalled );

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
        TomahawkSettings::instance()->setAtticaResolverState( resolver.id(), Uninstalled );
    }

    Tomahawk::Pipeline::instance()->removeScriptResolver( pathFromId( resolver.id() ) );
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
