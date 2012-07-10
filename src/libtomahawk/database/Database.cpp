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
#include "utils/Logger.h"
#include "Source.h"

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
{
    s_instance = this;

    if ( MAX_WORKER_THREADS < DEFAULT_WORKER_THREADS )
        m_maxConcurrentThreads = MAX_WORKER_THREADS;
    else
        m_maxConcurrentThreads = qBound( DEFAULT_WORKER_THREADS, QThread::idealThreadCount(), MAX_WORKER_THREADS );

    tDebug() << Q_FUNC_INFO << "Using" << m_maxConcurrentThreads << "database worker threads; current (GUI) thread is " << QThread::currentThread();

    connect( m_impl, SIGNAL( indexReady() ), SIGNAL( indexReady() ) );
    connect( m_impl, SIGNAL( indexReady() ), SIGNAL( ready() ) );
    connect( m_impl, SIGNAL( indexReady() ), SLOT( setIsReadyTrue() ) );

    m_workerRW->start();
}


Database::~Database()
{
    qDebug() << Q_FUNC_INFO;

    m_workerRW->quit();
    foreach ( DatabaseWorkerThread *thread, m_workerThreads )
    {
        if ( thread->worker() )
            thread->quit();
    }

    m_workerRW->wait( 60000 );
    delete m_workerRW;
    m_workerRW = 0;
    foreach ( DatabaseWorkerThread *thread, m_workerThreads )
    {
        thread->wait( 60000 );
        delete thread;
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
    qDebug() << "Enqueueing" << lc.count() << "commands to rw thread";
    if ( m_workerRW->worker() )
        m_workerRW->worker().data()->enqueue( lc );
}


void
Database::enqueue( const QSharedPointer<DatabaseCommand>& lc )
{
    Q_ASSERT( m_ready );
    if ( lc->doesMutates() )
    {
        qDebug() << "Enqueueing command to rw thread:" << lc->commandname();
        if ( m_workerRW->worker() )
            m_workerRW->worker().data()->enqueue( lc );
    }
    else
    {
        // find existing amount of worker threads for commandname
        // create new thread if < WORKER_THREADS
        if ( m_workerThreads.count() < m_maxConcurrentThreads )
        {
            DatabaseWorkerThread* workerThread = new DatabaseWorkerThread( this, false );
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "P1" << ( workerThread->worker() ? "true" : "false" );
            workerThread->start();
            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "P2" << ( workerThread->worker() ? "true" : "false" );

            m_workerThreads << workerThread;
        }

        // find thread for commandname with lowest amount of outstanding jobs and enqueue job
        int busyThreads = 0;
        QWeakPointer< DatabaseWorker > happyWorker;
        for ( int i = 0; i < m_workerThreads.count(); i++ )
        {
            DatabaseWorkerThread* workerThread = m_workerThreads.at( i );

            tDebug( LOGVERBOSE ) << Q_FUNC_INFO << i << ( workerThread->worker() ? "true" : "false" );
            if ( workerThread->worker() && !workerThread->worker().data()->busy() )
            {
                happyWorker = workerThread->worker();
                break;
            }
            busyThreads++;

            if ( !happyWorker || ( workerThread->worker() && workerThread->worker().data()->outstandingJobs() < happyWorker.data()->outstandingJobs() ) )
                happyWorker = workerThread->worker();
        }

//        qDebug() << "Enqueueing command to thread:" << happyThread << busyThreads << lc->commandname();
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << m_workerThreads.count() << m_maxConcurrentThreads;
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
        tDebug() << Q_FUNC_INFO << "Creating database impl for thread" << QThread::currentThread();
        DatabaseImpl* impl = m_impl->clone();
        m_implHash.insert( thread, impl );
    }

    return m_implHash.value( thread );
}
