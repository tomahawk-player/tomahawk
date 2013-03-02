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

#include "DatabaseWorker.h"

#include <QTimer>
#include <QTime>
#include <QSqlQuery>

#include "Source.h"
#include "Database.h"
#include "DatabaseImpl.h"
#include "DatabaseCommandLoggable.h"
#include "TomahawkSqlQuery.h"
#include "utils/Logger.h"

#ifndef QT_NO_DEBUG
    //#define DEBUG_TIMING TRUE
#endif


DatabaseWorkerThread::DatabaseWorkerThread( Database* db, bool mutates )
    : QThread()
    , m_db( db )
    , m_mutates( mutates )
{
}


void
DatabaseWorkerThread::run()
{
    tDebug() << Q_FUNC_INFO << "DatabaseWorkerThread starting...";
    m_worker = QPointer< DatabaseWorker >( new DatabaseWorker( m_db, m_mutates ) );
    exec();
    tDebug() << Q_FUNC_INFO << "DatabaseWorkerThread finishing...";
    if ( m_worker )
        delete m_worker.data();
}


DatabaseWorkerThread::~DatabaseWorkerThread()
{
}


QPointer< DatabaseWorker >
DatabaseWorkerThread::worker() const
{
    return m_worker;
}


DatabaseWorker::DatabaseWorker( Database* db, bool mutates )
    : QObject()
    , m_db( db )
    , m_outstanding( 0 )
{
    Q_UNUSED( mutates );
    tDebug() << Q_FUNC_INFO << "New db connection with name:" << Database::instance()->impl()->database().connectionName() << "on thread" << this->thread();
}


DatabaseWorker::~DatabaseWorker()
{
    tDebug() << Q_FUNC_INFO << m_outstanding;

    if ( m_outstanding )
    {
        foreach ( const QSharedPointer<DatabaseCommand>& cmd, m_commands )
        {
            tDebug() << "Outstanding db command to finish:" << cmd->guid() << cmd->commandname();
        }
    }
}


void
DatabaseWorker::enqueue( const QList< QSharedPointer<DatabaseCommand> >& cmds )
{
    QMutexLocker lock( &m_mut );
    m_outstanding += cmds.count();
    m_commands << cmds;

    if ( m_outstanding == cmds.count() )
        QTimer::singleShot( 0, this, SLOT( doWork() ) );
}


void
DatabaseWorker::enqueue( const QSharedPointer<DatabaseCommand>& cmd )
{
    QMutexLocker lock( &m_mut );
    m_outstanding++;
    m_commands << cmd;

    if ( m_outstanding == 1 )
        QTimer::singleShot( 0, this, SLOT( doWork() ) );
}


void
DatabaseWorker::doWork()
{
    /*
        Run the dbcmd. Only inside a transaction if the cmd does mutates.

        If the cmd is modifying local content (ie source->isLocal()) then
        log to the database oplog for replication to peers.

     */

#ifdef DEBUG_TIMING
    QTime timer;
    timer.start();
#endif

    QList< QSharedPointer<DatabaseCommand> > cmdGroup;
    QSharedPointer<DatabaseCommand> cmd;
    {
        QMutexLocker lock( &m_mut );
        cmd = m_commands.takeFirst();
    }

    DatabaseImpl* impl = Database::instance()->impl();
    if ( cmd->doesMutates() )
    {
        bool transok = impl->database().transaction();
        Q_ASSERT( transok );
        Q_UNUSED( transok );
    }

    unsigned int completed = 0;
    try
    {
        bool finished = false;
        {
            while ( !finished )
            {
                completed++;
                cmd->_exec( impl ); // runs actual SQL stuff

                if ( cmd->loggable() )
                {
                    // We only save our own ops to the oplog, since incoming ops from peers
                    // are applied immediately.
                    //
                    // Crazy idea: if peers had keypairs and could sign ops/msgs, in theory it
                    // would be safe to sync ops for friend A from friend B's cache, if he saved them,
                    // which would mean you could get updates even if a peer was offline.
                    if ( cmd->source()->isLocal() && !cmd->localOnly() )
                    {
                        // save to op-log
                        DatabaseCommandLoggable* command = (DatabaseCommandLoggable*)cmd.data();
                        logOp( command );
                    }
                    else
                    {
                        // Make a note of the last guid we applied for this source
                        // so we can always request just the newer ops in future.
                        //
                        if ( !cmd->singletonCmd() )
                        {
                            TomahawkSqlQuery query = impl->newquery();
                            query.prepare( "UPDATE source SET lastop = ? WHERE id = ?" );
                            query.addBindValue( cmd->guid() );
                            query.addBindValue( cmd->source()->id() );

                            if ( !query.exec() )
                            {
                                throw "Failed to set lastop";
                            }
                        }
                    }
                }

                cmdGroup << cmd;
                if ( cmd->groupable() && !m_commands.isEmpty() )
                {
                    QMutexLocker lock( &m_mut );
                    if ( m_commands.first()->groupable() )
                    {
                        cmd = m_commands.takeFirst();
                    }
                    else
                    {
                        finished = true;
                    }
                }
                else
                    finished = true;
            }

            if ( cmd->doesMutates() )
            {
                qDebug() << "Committing" << cmd->commandname() << cmd->guid();
                if ( !impl->newquery().commitTransaction() )
                {
                    tDebug() << "FAILED TO COMMIT TRANSACTION*";
                    throw "commit failed";
                }
            }

#ifdef DEBUG_TIMING
            uint duration = timer.elapsed();
            tDebug() << "DBCmd Duration:" << duration << "ms, now running postcommit for" << cmd->commandname();
#endif

            foreach ( QSharedPointer<DatabaseCommand> c, cmdGroup )
                c->postCommit();

#ifdef DEBUG_TIMING
            tDebug() << "Post commit finished in" << timer.elapsed() - duration << "ms for" << cmd->commandname();
#endif
        }
    }
    catch ( const char * msg )
    {
        tLog() << endl
                 << "*ERROR* processing databasecommand:"
                 << cmd->commandname()
                 << msg
                 << impl->database().lastError().databaseText()
                 << impl->database().lastError().driverText()
                 << endl;

        if ( cmd->doesMutates() )
            impl->database().rollback();

        Q_ASSERT( false );
    }
    catch (...)
    {
        qDebug() << "Uncaught exception processing dbcmd";
        if ( cmd->doesMutates() )
            impl->database().rollback();

        Q_ASSERT( false );
        throw;
    }

    foreach ( QSharedPointer<DatabaseCommand> c, cmdGroup )
        c->emitFinished();

    QMutexLocker lock( &m_mut );
    m_outstanding -= completed;
    if ( m_outstanding > 0 )
        QTimer::singleShot( 0, this, SLOT( doWork() ) );
}


// this should take a const command, need to check/make json stuff mutable for some objs tho maybe.
void
DatabaseWorker::logOp( DatabaseCommandLoggable* command )
{
    TomahawkSqlQuery oplogquery = Database::instance()->impl()->newquery();
    qDebug() << "INSERTING INTO OPLOG:" << command->source()->id() << command->guid() << command->commandname();
    oplogquery.prepare( "INSERT INTO oplog(source, guid, command, singleton, compressed, json) "
                        "VALUES(?, ?, ?, ?, ?, ?)" );

    QVariantMap variant = QJson::QObjectHelper::qobject2qvariant( command );
    QByteArray ba = m_serializer.serialize( variant );

//     qDebug() << "OP JSON:" << ba.isNull() << ba << "from:" << variant; // debug

    bool compressed = false;
    if ( ba.length() >= 512 )
    {
        // We need to compress this in this thread, since inserting into the log
        // has to happen as part of the same transaction as the dbcmd.
        // (we are in a worker thread for RW dbcmds anyway, so it's ok)
        //qDebug() << "Compressing DB OP JSON, uncompressed size:" << ba.length();
        ba = qCompress( ba, 9 );
        compressed = true;
        //qDebug() << "Compressed DB OP JSON size:" << ba.length();
    }

    if ( command->singletonCmd() )
    {
        tDebug() << "Singleton command, deleting previous oplog commands";

        TomahawkSqlQuery oplogdelquery = Database::instance()->impl()->newquery();
        oplogdelquery.prepare( QString( "DELETE FROM oplog WHERE source %1 AND singleton = 'true' AND command = ?" )
                                  .arg( command->source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( command->source()->id() ) ) );

        oplogdelquery.bindValue( 0, command->commandname() );
        oplogdelquery.exec();
    }

    tDebug() << "Saving to oplog:" << command->commandname()
             << "bytes:" << ba.length()
             << "guid:" << command->guid();

    oplogquery.bindValue( 0, command->source()->isLocal() ?
                          QVariant(QVariant::Int) : command->source()->id() );
    oplogquery.bindValue( 1, command->guid() );
    oplogquery.bindValue( 2, command->commandname() );
    oplogquery.bindValue( 3, command->singletonCmd() );
    oplogquery.bindValue( 4, compressed );
    oplogquery.bindValue( 5, ba );
    if ( !oplogquery.exec() )
    {
        tLog() << "Error saving to oplog";
        throw "Failed to save to oplog";
    }
}
