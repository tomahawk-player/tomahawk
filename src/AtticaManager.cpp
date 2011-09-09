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

#include "utils/logger.h"
#include "tomahawksettings.h"
#include "utils/tomahawkutils.h"

#include <attica/downloaditem.h>
#include <quazip/quazip.h>
#include <quazip/quazipfile.h>

#include <QNetworkReply>
#include <QtCore/qtemporaryfile.h>
#include <QDir>
#include "tomahawkapp.h"

using namespace Attica;

AtticaManager* AtticaManager::s_instance = 0;

AtticaManager::AtticaManager( QObject* parent )
{
    connect( &m_manager, SIGNAL( providerAdded( Attica::Provider ) ), this, SLOT( providerAdded( Attica::Provider ) ) );

    // resolvers
    m_manager.addProviderFile( QUrl( "http://bakery.tomahawk-player.org:10480/resolvers/providers.xml" ) );
}

AtticaManager::~AtticaManager()
{

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

    return m_resolverStates[ resolver.id() ];
}

bool
AtticaManager::resolversLoaded() const
{
    return !m_resolvers.isEmpty();
}

QString
AtticaManager::pathFromId( const QString& resolverId ) const
{
    foreach( const Content& content, m_resolvers )
    {
        if ( content.id() == resolverId )
            return QString( "%1/%2/contents/code/main.js" ).arg( TomahawkUtils::appDataDir().absolutePath() ).arg( QString( "atticaresolvers/%1" ).arg( resolverId ) );
    }

    return QString();
}


void
AtticaManager::providerAdded( const Provider& provider )
{
    if ( provider.name() == "Tomahawk Resolvers" )
    {
        m_resolverProvider = provider;

        ListJob< Content >* job = m_resolverProvider.searchContents( Category::List(), QString(), Provider::Rating );
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
}

void
AtticaManager::installResolver( const Content& resolver )
{
    Q_ASSERT( !resolver.id().isNull() );

    m_resolverStates[ resolver.id() ] = Installing;
    emit resolverStateChanged( resolver.id() );

    ItemJob< DownloadItem >* job = m_resolverProvider.downloadLink( resolver.id() );
    connect( job, SIGNAL( finished( Attica::BaseJob* ) ), this, SLOT( resolverDownloadFinished( Attica::BaseJob* ) ) );
    job->setProperty( "resolverId", resolver.id() );

    job->start();
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
        QString resolverPath = extractPayload( f.fileName(), resolverId );

        if( !resolverPath.isEmpty() )
        {
//             TomahawkApp::instance()->enableScriptResolver( resolverPath );
            m_resolverStates[ resolverId ] = Installed;
            TomahawkSettings::instance()->setAtticaResolverState( resolverId, Installed );
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

    // The path is *always* resovlerid/contents/code/main.js
    return QString( QFile( resolverDir.absolutePath() + "/contents/code/main.js" ).fileName() );
}


void
AtticaManager::uninstallResolver( const Content& resolver )
{
    TomahawkApp::instance()->disableScriptResolver( resolver.id() );
    m_resolverStates[ resolver.id() ] = Uninstalled;

    emit resolverUninstalled( resolver.id() );
    emit resolverStateChanged( resolver.id() );

    // uninstalling is easy... just delete it! :)
    QDir resolverDir = TomahawkUtils::appDataDir();
    resolverDir.cd( QString( "atticaresolvers/%1" ).arg( resolver.id() ) );
    removeDirectory( resolverDir.absolutePath() );

}

// taken from util/fileutils.cpp in kdevplatform
bool
AtticaManager::removeDirectory( const QString& dir ) const
{
    const QDir aDir(dir);

    bool has_err = false;
    if (aDir.exists()) {
        foreach(const QFileInfo& entry, aDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::NoSymLinks)) {
            QString path = entry.absoluteFilePath();
            if (entry.isDir()) {
                has_err = !removeDirectory(path) || has_err;
            } else if (!QFile::remove(path)) {
                has_err = true;
            }
        }
        if (!aDir.rmdir(aDir.absolutePath())) {
            has_err = true;
        }
    }
    return !has_err;
}