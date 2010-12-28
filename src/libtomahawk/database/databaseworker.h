#ifndef DATABASEWORKER_H
#define DATABASEWORKER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QList>
#include <QSharedPointer>

#include <qjson/parser.h>
#include <qjson/serializer.h>
#include <qjson/qobjecthelper.h>

#include "databasecommand.h"
#include "databaseimpl.h"

class Database;
class DatabaseCommandLoggable;

class DatabaseWorker : public QThread
{
Q_OBJECT

public:
    DatabaseWorker( DatabaseImpl*, Database*, bool mutates );
    ~DatabaseWorker();
    //void enqueue( QSharedPointer<DatabaseCommand> );

protected:
    void run();

public slots:
    void doWork( QSharedPointer<DatabaseCommand> );

private:
    void logOp( DatabaseCommandLoggable* command );

    QMutex m_mut;
    DatabaseImpl* m_dbimpl;
    QList< QSharedPointer<DatabaseCommand> > m_commands;
    bool m_abort;
    int m_outstanding;
    QJson::Serializer m_serializer;
};

#endif // DATABASEWORKER_H
