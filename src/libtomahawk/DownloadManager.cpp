/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "DownloadManager.h"

#include <QTimer>

#include "filemetadata/ScanManager.h"
#include "TomahawkSettings.h"
#include "infosystem/InfoSystem.h"
#include "utils/Logger.h"
#include "Result.h"
#include "Query.h"

DownloadManager* DownloadManager::s_instance = 0;


DownloadManager*
DownloadManager::instance()
{
    if ( !s_instance )
    {
        s_instance = new DownloadManager();
    }

    return s_instance;
}


DownloadManager::DownloadManager()
    : m_globalState( true )
{
    tLog() << Q_FUNC_INFO << "Initializing DownloadManager.";

    QVariantList downloads = TomahawkSettings::instance()->downloadStates();
    foreach ( const QVariant& download, downloads )
    {
        QVariantMap map = download.toMap();
        QString localFile = map[ "localfile" ].toString();
        QFileInfo fi( localFile );
        if ( fi.exists() )
            m_downloadStates[ map[ "url" ].toString() ] = map;
    }

    QTimer::singleShot( 0, this, SLOT( resumeJobs() ) );
}


DownloadManager::~DownloadManager()
{
    tLog() << Q_FUNC_INFO << "Shutting down DownloadManager.";

    storeJobs( jobs( DownloadJob::Finished ) );
}


QString
DownloadManager::localFileForDownload( const QString& url ) const
{
    if ( m_downloadStates.contains( url ) )
    {
        QVariantMap map = m_downloadStates[ url ];
//        tDebug() << "Found known download:" << map["url"] << map["localfile"];
//        tDebug() << "Looking for download:" << url;
        QString localFile = map[ "localfile" ].toString();
        QFileInfo fi( localFile );
        if ( fi.exists() )
            return localFile;
    }

    return QString();
}


QUrl
DownloadManager::localUrlForDownload( const Tomahawk::query_ptr& query ) const
{
    Tomahawk::result_ptr result = query->numResults( true ) ? query->results().first() : Tomahawk::result_ptr();
    if ( result )
    {
        return localUrlForDownload( result );
    }

    return QUrl();
}


QUrl
DownloadManager::localUrlForDownload( const Tomahawk::result_ptr& result ) const
{
    if ( result && !result->downloadFormats().isEmpty() &&
        !localFileForDownload( result->downloadFormats().first().url.toString() ).isEmpty() )
    {
        return QUrl::fromLocalFile( QFileInfo( DownloadManager::instance()->localFileForDownload( result->downloadFormats().first().url.toString() ) ).absolutePath() );
    }
    else if ( result && result->downloadJob() && result->downloadJob()->state() == DownloadJob::Finished )
    {
        return QUrl::fromLocalFile( QFileInfo( result->downloadJob()->localFile() ).absolutePath() );
    }

    return QUrl();
}


void
DownloadManager::storeJobs( const QList<downloadjob_ptr>& jobs )
{
    tDebug() << Q_FUNC_INFO;
    QVariantList downloads = TomahawkSettings::instance()->downloadStates();
    foreach ( const downloadjob_ptr& job, jobs )
    {
        if ( job->state() != DownloadJob::Finished )
            continue;
        tDebug() << Q_FUNC_INFO << "Storing job:" << job->format().url << job->localFile();
        QVariantMap map;
        map[ "url" ] = job->format().url;
        map[ "localfile" ] = job->localFile();

        m_downloadStates[ map[ "url" ].toString() ] = map;

        bool dupe = false;
        foreach ( const QVariant& m, downloads )
        {
            if ( m.toMap()[ "url" ] == map[ "url" ] )
                dupe = true;
        }

        if ( !dupe )
            downloads << map;
    }

    TomahawkSettings::instance()->setDownloadStates( downloads );
}


void
DownloadManager::resumeJobs()
{
    tLog() << Q_FUNC_INFO;

/*    QList<downloadjob_ptr> jobs = TomahawkSettings::instance()->storedJobs();
    for ( int i = jobs.count() - 1; i >= 0; i-- )
    {
        downloadjob_ptr job = jobs.at( i );
        addJob( job );
    }

    tLog() << Q_FUNC_INFO << "Restored" << jobs.count() << "existing jobs.";*/
}


QList<downloadjob_ptr>
DownloadManager::jobs( DownloadJob::TrackState state ) const
{
    if ( state < 0 )
        return m_jobs;

    QList<downloadjob_ptr> jobs;
    foreach ( const downloadjob_ptr& job, m_jobs )
    {
        if ( job.isNull() )
            continue;

        if ( state == DownloadJob::Any || job->state() == state )
            jobs << job;
    }

    return jobs;
}


bool
DownloadManager::addJob( const downloadjob_ptr& job )
{
    if ( job.isNull() )
    {
        tLog() << "Found invalid download job - ignoring!";
        return false;
    }

    if ( containsJob( job ) )
    {
        tLog() << "Found duplicate download job - ignoring:" << job->toString();
        return false;
    }

    m_jobs << job;
    emit jobAdded( job );

    connect( job.data(), SIGNAL( finished() ), SLOT( onJobFinished() ) );
    connect( job.data(), SIGNAL( finished() ), SLOT( checkJobs() ) );
    connect( job.data(), SIGNAL( stateChanged( DownloadJob::TrackState, DownloadJob::TrackState ) ), SLOT( checkJobs() ) ) ;
//    connect( job.data(), SIGNAL( stateChanged( DownloadJob::TrackState, DownloadJob::TrackState ) ), SIGNAL( stateChanged( DownloadJob::TrackState, DownloadJob::TrackState ) ) );

    checkJobs();
    return true;
}


bool
DownloadManager::removeJob( const downloadjob_ptr& job )
{
    tLog() << "Removing job:" << job->toString();
    job->abort();
    m_jobs.removeAll( job );
    emit jobRemoved( job );

    checkJobs();

    return true;
}


bool
DownloadManager::containsJob( const downloadjob_ptr& job ) const
{
    return m_jobs.contains( job );
}


downloadjob_ptr
DownloadManager::currentJob() const
{
    QList<downloadjob_ptr> j = jobs( DownloadJob::Running );
    if ( !j.isEmpty() )
        return j.first();

    j = jobs( DownloadJob::Paused );
    if ( !j.isEmpty() )
        return j.first();

    j = jobs( DownloadJob::Waiting );
    if ( !j.isEmpty() )
        return j.first();

    return downloadjob_ptr();
}



DownloadManager::DownloadManagerState
DownloadManager::state() const
{
    if ( !currentJob().isNull() )
    {
        switch ( currentJob()->state() )
        {
            case DownloadJob::Waiting:
                return DownloadManager::Waiting;

            case DownloadJob::Paused:
                return DownloadManager::Paused;

            case DownloadJob::Running:
                return DownloadManager::Running;

            default:
                break;
        }
    }

    return DownloadManager::Waiting;
}


void
DownloadManager::checkJobs()
{
    if ( !m_globalState )
        return;

    if ( state() == DownloadManager::Waiting )
    {
        if ( !currentJob().isNull() )
        {
            downloadjob_ptr job = currentJob();
    /*        connect( job.data(), SIGNAL( finished() ), SLOT( checkJobs() ) );
            connect( job.data(), SIGNAL( stateChanged( DownloadJob::TrackState, DownloadJob::TrackState ) ), SLOT( checkJobs() ) ) ;
            connect( job.data(), SIGNAL( stateChanged( DownloadJob::TrackState, DownloadJob::TrackState ) ), SIGNAL( stateChanged( Track::TrackState, Track::TrackState ) ) );*/

            job->download();

            emit stateChanged( DownloadManager::Running, DownloadManager::Waiting );
        }
        else
        {
            emit stateChanged( DownloadManager::Waiting , DownloadManager::Running );
        }
    }
}


void
DownloadManager::onJobFinished()
{
    DownloadJob* job = qobject_cast<DownloadJob*>( sender() );

    QStringList files;
    files << job->localFile();
    ScanManager::instance()->runFileScan( files, true );

    Tomahawk::InfoSystem::InfoPushData pushData( "DownloadManager", Tomahawk::InfoSystem::InfoNotifyUser,
                                                 tr( "%applicationName finished downloading %1 by %2." ).arg( job->track()->track() ).arg( job->track()->artist() ),
                                                 Tomahawk::InfoSystem::PushNoFlag );
    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( pushData );

    storeJobs( jobs( DownloadJob::Finished ) );
}


void
DownloadManager::cancelAll()
{
    foreach ( const downloadjob_ptr& job, jobs( DownloadJob::Waiting ) )
    {
        removeJob( job );
    }
    foreach ( const downloadjob_ptr& job, jobs( DownloadJob::Running ) )
    {
        job->abort();
    }
}


void
DownloadManager::pause()
{
    tLog() << Q_FUNC_INFO;

    m_globalState = false;
    foreach ( const downloadjob_ptr& job, jobs( DownloadJob::Running ) )
    {
        job->pause();
    }
}


void
DownloadManager::resume()
{
    tLog() << Q_FUNC_INFO;

    m_globalState = true;

    if ( !jobs( DownloadJob::Paused ).isEmpty() )
    {
        foreach ( const downloadjob_ptr& job, jobs( DownloadJob::Paused ) )
        {
            tLog() << "Resuming job:" << job->toString();
            job->resume();
        }

        return;
    }

    checkJobs();
}
