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
    the queue of work. There is a single thread responsible for exec'ing all
    the commands, so sqlite only does one thing at a time.

    Update: 1 thread for mutates, one for readonly queries.
*/
class DLLEXPORT Database : public QObject
{
Q_OBJECT
public:
    static Database* instance();

    explicit Database( const QString& dbname, QObject* parent = 0 );
    ~Database();

    const QString& dbid() const;
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
    DatabaseWorker *m_workerRO, *m_workerRW;
    bool m_indexReady;

    static Database* s_instance;
};

#endif // DATABASE_H
