/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "Database.h"

#include "DatabaseCommand.h"
#include "DatabaseImpl.h"
#include "DatabaseWorker.h"
#include "IdThreadWorker.h"
#include "utils/Logger.h"

#define DEFAULT_WORKER_THREADS 4
#define MAX_WORKER_THREADS 16

Database* Database::s_instance = 0;


Database*
Database::instance()
{
    return s_instance;
}


Database::Database( const QString& dbname, QObject* parent )
    : QObject( parent )
    , m_ready( false )
    , m_impl( new DatabaseImpl( dbname ) )
    , m_workerRW( new DatabaseWorkerThread( this, true ) )
    , m_idWorker( new IdThreadWorker( this ) )
{
    s_instance = this;

    if ( MAX_WORKER_THREADS < DEFAULT_WORKER_THREADS )
        m_maxConcurrentThreads = MAX_WORKER_THREADS;
    else
        m_maxConcurrentThreads = qBound( DEFAULT_WORKER_THREADS, QThread::idealThreadCount(), MAX_WORKER_THREADS );

    tDebug() << Q_FUNC_INFO << "Using" << m_maxConcurrentThreads << "database worker threads";

    connect( m_impl, SIGNAL( indexReady() ), SLOT( markAsReady() ) );
    connect( m_impl, SIGNAL( indexReady() ), SIGNAL( indexReady() ) );
    connect( m_impl, SIGNAL( indexReady() ), SIGNAL( ready() ) );

    Q_ASSERT( m_workerRW );
    m_workerRW.data()->start();

    while ( m_workerThreads.count() < m_maxConcurrentThreads )
    {
        QPointer< DatabaseWorkerThread > workerThread( new DatabaseWorkerThread( this, false ) );
        Q_ASSERT( workerThread );
        workerThread.data()->start();
        m_workerThreads << workerThread;
    }
    m_idWorker->start();
}


Database::~Database()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;

    m_idWorker->stop();
    delete m_idWorker;

    if ( m_workerRW )
        m_workerRW.data()->quit();
    foreach ( QPointer< DatabaseWorkerThread > workerThread, m_workerThreads )
    {
        if ( workerThread && workerThread.data()->worker() )
            workerThread.data()->quit();
    }

    if ( m_workerRW )
    {
        m_workerRW.data()->wait( 60000 );
        delete m_workerRW.data();
    }
    foreach ( QPointer< DatabaseWorkerThread > workerThread, m_workerThreads )
    {
        if ( workerThread )
        {
            workerThread.data()->wait( 60000 );
            delete workerThread.data();
        }
    }
    m_workerThreads.clear();

    qDeleteAll( m_implHash.values() );
    delete m_impl;

}


void
Database::loadIndex()
{
    m_impl->loadIndex();
}


void
Database::enqueue( const QList< QSharedPointer<DatabaseCommand> >& lc )
{
    Q_ASSERT( m_ready );
    if ( !m_ready )
    {
        tDebug() << "Can't enqueue DatabaseCommand, Database is not ready yet!";
        return;
    }

    tDebug( LOGVERBOSE ) << "Enqueueing" << lc.count() << "commands to rw thread";
    if ( m_workerRW && m_workerRW.data()->worker() )
        m_workerRW.data()->worker().data()->enqueue( lc );
}


void
Database::enqueue( const QSharedPointer<DatabaseCommand>& lc )
{
    Q_ASSERT( m_ready );
    if ( !m_ready )
    {
        tDebug() << "Can't enqueue DatabaseCommand, Database is not ready yet!";
        return;
    }

    if ( lc->doesMutates() )
    {
        tDebug( LOGVERBOSE ) << "Enqueueing command to rw thread:" << lc->commandname();
        if ( m_workerRW && m_workerRW.data()->worker() )
            m_workerRW.data()->worker().data()->enqueue( lc );
    }
    else
    {
        // find thread for commandname with lowest amount of outstanding jobs and enqueue job
        int busyThreads = 0;
        QPointer< DatabaseWorkerThread > workerThread;
        QPointer< DatabaseWorker > happyWorker;
        for ( int i = 0; i < m_workerThreads.count(); i++ )
        {
            workerThread = m_workerThreads.at( i );

            if ( workerThread && workerThread.data()->worker() && !workerThread.data()->worker().data()->busy() )
            {
                happyWorker = workerThread.data()->worker();
                break;
            }
            busyThreads++;

            if ( ( !happyWorker && workerThread && workerThread.data()->worker() ) ||
                 ( workerThread && workerThread.data()->worker() && workerThread.data()->worker().data()->outstandingJobs() < happyWorker.data()->outstandingJobs() ) )
                happyWorker = workerThread.data()->worker();
        }

        tDebug( LOGVERBOSE ) << "Enqueueing command to thread:" << happyWorker << busyThreads << lc->commandname();
        Q_ASSERT( happyWorker );
        happyWorker.data()->enqueue( lc );
    }
}


DatabaseImpl*
Database::impl()
{
    QMutexLocker lock( &m_mutex );

    QThread* thread = QThread::currentThread();
    if ( !m_implHash.contains( thread ) )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Creating database impl for thread" << QThread::currentThread();
        DatabaseImpl* impl = m_impl->clone();
        m_implHash.insert( thread, impl );
    }

    return m_implHash.value( thread );
}


void
Database::markAsReady()
{
    tLog() << Q_FUNC_INFO << "Database is ready now!";
    m_ready = true;
}
