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
