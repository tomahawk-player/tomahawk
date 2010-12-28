#include "database.h"

Database* Database::s_instance = 0;


Database*
Database::instance()
{
    return s_instance;
}


Database::Database( const QString& dbname, QObject* parent )
    : QObject( parent )
    , m_impl( new DatabaseImpl( dbname, this ) )
    , m_workerRO( new DatabaseWorker( m_impl, this, false ) )
    , m_workerRW( new DatabaseWorker( m_impl, this, true ) )
{
    s_instance = this;

    m_workerRO->start();
    m_workerRW->start();
}


Database::~Database()
{
    qDebug() << Q_FUNC_INFO;

    delete m_workerRW;
    delete m_workerRO;
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
        emit newJobRO( lc );
    }
    else
    {
        //qDebug() << Q_FUNC_INFO << "RO" << lc->commandname();
        emit newJobRW( lc );
    }
}


const QString&
Database::dbid() const
{
    return m_impl->dbid();
}
