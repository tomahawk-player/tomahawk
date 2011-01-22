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

    for ( int i = 0; i < WORKER_THREADS; i++ )
    {
        DatabaseWorker* worker = new DatabaseWorker( m_impl, this, false );
        worker->start();

        m_workersRO << worker;
    }

    m_workerRW->start();
}


Database::~Database()
{
    qDebug() << Q_FUNC_INFO;

    qDeleteAll( m_workersRO );
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
//        qDebug() << Q_FUNC_INFO << "RO" << lc->commandname();

        int busyThreads = 0;
        DatabaseWorker* happyThread = 0;
        for ( int i = 0; i < m_workersRO.count(); i++ )
        {
            DatabaseWorker* worker = m_workersRO.at( i );

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


const QString&
Database::dbid() const
{
    return m_impl->dbid();
}
