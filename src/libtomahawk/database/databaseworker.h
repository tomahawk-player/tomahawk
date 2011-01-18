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

    void enqueue( const QSharedPointer<DatabaseCommand>& );

    bool busy() const { return m_outstanding > 0; }
    unsigned int outstandingJobs() const { return m_outstanding; }

protected:
    void run();

private slots:
    void doWork();

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
