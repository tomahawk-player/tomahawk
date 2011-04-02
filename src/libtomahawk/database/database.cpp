/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "database.h"

#define WORKER_THREADS 5

Database* Database::s_instance = 0;


Database*
Database::instance()
{
    return s_instance;
}


Database::Database( const QString& dbname, QObject* parent )
    : QObject( parent )
    , m_impl( new DatabaseImpl( dbname, this ) )
    , m_workerRW( new DatabaseWorker( m_impl, this, true ) )
{
    s_instance = this;

    connect( m_impl, SIGNAL( indexReady() ), SIGNAL( indexReady() ) );
    connect( m_impl, SIGNAL( indexReady() ), SIGNAL( ready() ) );

    m_workerRW->start();
}


Database::~Database()
{
    qDebug() << Q_FUNC_INFO;

    qDeleteAll( m_workers );
    delete m_workerRW;
    delete m_impl;
}


void
Database::loadIndex()
{
    m_impl->loadIndex();
}


void
Database::enqueue( QSharedPointer<DatabaseCommand> lc )
{
    if( lc->doesMutates() )
    {
        //qDebug() << Q_FUNC_INFO << "RW" << lc->commandname();
        qDebug() << "Enqueueing command to rw thread:" << lc->commandname();
        m_workerRW->enqueue( lc );
    }
    else
    {
        // find existing amount of worker threads for commandname
        // create new thread if < WORKER_THREADS
        if ( m_workers.count( lc->commandname() ) < WORKER_THREADS )
        {
            DatabaseWorker* worker = new DatabaseWorker( m_impl, this, false );
            worker->start();

            m_workers.insertMulti( lc->commandname(), worker );
        }

        // find thread for commandname with lowest amount of outstanding jobs and enqueue job
        int busyThreads = 0;
        DatabaseWorker* happyThread = 0;
        QList< DatabaseWorker* > workers = m_workers.values( lc->commandname() );
        for ( int i = 0; i < workers.count(); i++ )
        {
            DatabaseWorker* worker = workers.at( i );

            if ( !worker->busy() )
            {
                happyThread = worker;
                break;
            }
            busyThreads++;

            if ( !happyThread || worker->outstandingJobs() < happyThread->outstandingJobs() )
                happyThread = worker;
        }

        qDebug() << "Enqueueing command to thread:" << happyThread << busyThreads << lc->commandname();
        happyThread->enqueue( lc );
    }
}


QString
Database::dbid() const
{
    return m_impl->dbid();
}
