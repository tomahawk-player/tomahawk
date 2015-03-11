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

#include "utils/Logger.h"

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

    QTimer::singleShot( 0, this, SLOT( resumeJobs() ) );
}


DownloadManager::~DownloadManager()
{
    tLog() << Q_FUNC_INFO << "Shutting down DownloadManager.";

    QList< downloadjob_ptr > jl;
    foreach ( const downloadjob_ptr& job, jobs() )
    {
        jl << job;
    }

//    TomahawkSettings::instance()->storeJobs( jl );
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

        if ( state == DownloadJob::TrackState::Any || job->state() == state )
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
    QList<downloadjob_ptr> j = jobs( DownloadJob::TrackState::Running );
    if ( j.count() )
        return j.first();

    j = jobs( DownloadJob::TrackState::Paused );
    if ( j.count() )
        return j.first();

    j = jobs( DownloadJob::TrackState::Waiting );
    if ( j.count() )
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
            case DownloadJob::TrackState::Waiting:
                return DownloadManager::DownloadManagerState::Waiting;

            case DownloadJob::TrackState::Paused:
                return DownloadManager::DownloadManagerState::Paused;

            case DownloadJob::TrackState::Running:
                return DownloadManager::DownloadManagerState::Running;
        }
    }

    return DownloadManager::DownloadManagerState::Waiting;
}


void
DownloadManager::checkJobs()
{
    if ( !m_globalState )
        return;

    if ( state() == DownloadManager::DownloadManagerState::Waiting && !currentJob().isNull() )
    {
        downloadjob_ptr job = currentJob();
/*        connect( job.data(), SIGNAL( finished() ), SLOT( checkJobs() ) );
        connect( job.data(), SIGNAL( stateChanged( DownloadJob::TrackState, DownloadJob::TrackState ) ), SLOT( checkJobs() ) ) ;
        connect( job.data(), SIGNAL( stateChanged( DownloadJob::TrackState, DownloadJob::TrackState ) ), SIGNAL( stateChanged( Track::TrackState, Track::TrackState ) ) );*/

        job->download();
    }
}


void
DownloadManager::pause()
{
    tLog() << Q_FUNC_INFO;

    m_globalState = false;
    foreach ( const downloadjob_ptr& job, jobs( DownloadJob::TrackState::Running ) )
    {
        job->pause();
    }
}


void
DownloadManager::resume()
{
    tLog() << Q_FUNC_INFO;

    m_globalState = true;

    if ( jobs( DownloadJob::TrackState::Paused ).count() )
    {
        foreach ( const downloadjob_ptr& job, jobs( DownloadJob::TrackState::Paused ) )
        {
            tLog() << "Resuming job:" << job->toString();
            job->resume();
        }

        return;
    }

    checkJobs();
}
