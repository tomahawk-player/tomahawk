#ifndef DATABASE_H
#define DATABASE_H

#include <QSharedPointer>
#include <QVariant>

#include "database/databaseimpl.h"
#include "database/databasecommand.h"
#include "database/databaseworker.h"

#include "dllmacro.h"

/*
    This class is really a firewall/pimpl - the public functions of LibraryImpl
    are the ones that operate on the database, without any locks.

    HOWEVER, we're using the command pattern to serialize access to the database
    and provide an async api. You create a DatabaseCommand object, and add it to
    the queue of work. There is a threadpool responsible for exec'ing all
    the non-mutating (readonly) commands and one separate thread for mutating ones,
    so sqlite doesn't write to the Database from multiple threads.
*/
class DLLEXPORT Database : public QObject
{
Q_OBJECT
public:
    static Database* instance();

    explicit Database( const QString& dbname, QObject* parent = 0 );
    ~Database();

    QString dbid() const;
    const bool indexReady() const { return m_indexReady; }

    void loadIndex();

signals:
    void indexReady(); // search index
    void newJobRO( QSharedPointer<DatabaseCommand> );
    void newJobRW( QSharedPointer<DatabaseCommand> );

public slots:
    void enqueue( QSharedPointer<DatabaseCommand> lc );

private:
    DatabaseImpl* m_impl;
    DatabaseWorker* m_workerRW;
    QHash< QString, DatabaseWorker* > m_workers;
    bool m_indexReady;

    static Database* s_instance;
};

#endif // DATABASE_H
