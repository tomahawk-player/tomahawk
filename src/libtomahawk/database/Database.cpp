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
    , m_impl( new DatabaseImpl( dbname, this ) )
    , m_workerRW( new DatabaseWorker( this, true ) )
    , m_idWorker( new IdThreadWorker( this ) )
{
    s_instance = this;

    if ( MAX_WORKER_THREADS < DEFAULT_WORKER_THREADS )
        m_maxConcurrentThreads = MAX_WORKER_THREADS;
    else
        m_maxConcurrentThreads = qBound( DEFAULT_WORKER_THREADS, QThread::idealThreadCount(), MAX_WORKER_THREADS );

    tDebug() << Q_FUNC_INFO << "Using" << m_maxConcurrentThreads << "database worker threads";

    connect( m_impl, SIGNAL( indexReady() ), SIGNAL( indexReady() ) );
    connect( m_impl, SIGNAL( indexReady() ), SIGNAL( ready() ) );
    connect( m_impl, SIGNAL( indexReady() ), SLOT( setIsReadyTrue() ) );

    m_workerRW->start();
    m_idWorker->start();
}


Database::~Database()
{
    qDebug() << Q_FUNC_INFO;

    qDeleteAll( m_workers );
    delete m_workerRW;
    m_idWorker->stop();
    delete m_idWorker;
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
    qDebug() << "Enqueueing" << lc.count() << "commands to rw thread";
    m_workerRW->enqueue( lc );
}


void
Database::enqueue( const QSharedPointer<DatabaseCommand>& lc )
{
    if ( lc->doesMutates() )
    {
        qDebug() << "Enqueueing command to rw thread:" << lc->commandname();
        m_workerRW->enqueue( lc );
    }
    else
    {
        // find existing amount of worker threads for commandname
        // create new thread if < WORKER_THREADS
        if ( m_workers.count() < m_maxConcurrentThreads )
        {
            DatabaseWorker* worker = new DatabaseWorker( this, false );
            worker->start();

            m_workers << worker;
        }

        // find thread for commandname with lowest amount of outstanding jobs and enqueue job
        int busyThreads = 0;
        DatabaseWorker* happyThread = 0;
        for ( int i = 0; i < m_workers.count(); i++ )
        {
            DatabaseWorker* worker = m_workers.at( i );

            if ( !worker->busy() )
            {
                happyThread = worker;
                break;
            }
            busyThreads++;

            if ( !happyThread || worker->outstandingJobs() < happyThread->outstandingJobs() )
                happyThread = worker;
        }

//        qDebug() << "Enqueueing command to thread:" << happyThread << busyThreads << lc->commandname();
        happyThread->enqueue( lc );
    }
}


QString
Database::dbid() const
{
    return m_impl->dbid();
}
