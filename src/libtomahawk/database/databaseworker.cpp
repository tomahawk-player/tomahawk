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

#include "databaseworker.h"

#include <QTimer>
#include <QTime>
#include <QSqlQuery>

#include "database/database.h"
#include "database/databasecommandloggable.h"


DatabaseWorker::DatabaseWorker( DatabaseImpl* lib, Database* db, bool mutates )
    : QThread()
    , m_dbimpl( lib )
    , m_abort( false )
    , m_outstanding( 0 )
{
    Q_UNUSED( db );
    Q_UNUSED( mutates );

    moveToThread( this );

    qDebug() << "CTOR DatabaseWorker" << this->thread();
}


DatabaseWorker::~DatabaseWorker()
{
    qDebug() << Q_FUNC_INFO << m_outstanding;

    if ( m_commands.count() )
        qDebug() << m_commands;

    quit();
    wait( 60000 );
}


void
DatabaseWorker::run()
{
    exec();
    qDebug() << Q_FUNC_INFO << "DatabaseWorker finishing...";
}


void
DatabaseWorker::enqueue( const QSharedPointer<DatabaseCommand>& cmd )
{
    if ( QThread::currentThread() != thread() )
    {
//        qDebug() << Q_FUNC_INFO << "Reinvoking in correct thread.";
        QMetaObject::invokeMethod( this, "enqueue", Qt::QueuedConnection, Q_ARG( QSharedPointer<DatabaseCommand>, cmd ) );
        return;
    }

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

    QTime timer;
    timer.start();

    QSharedPointer<DatabaseCommand> cmd;
    {
        QMutexLocker lock( &m_mut );
        cmd = m_commands.takeFirst();
    }

    if( cmd->doesMutates() )
    {
        bool transok = m_dbimpl->database().transaction();
//        Q_ASSERT( transok );
        Q_UNUSED( transok );
    }
    try
    {
        {
            cmd->_exec( m_dbimpl ); // runs actual SQL stuff

            if ( cmd->loggable() && !cmd->localOnly() )
            {
                // We only save our own ops to the oplog, since incoming ops from peers
                // are applied immediately.
                //
                // Crazy idea: if peers had keypairs and could sign ops/msgs, in theory it
                // would be safe to sync ops for friend A from friend B's cache, if he saved them,
                // which would mean you could get updates even if a peer was offline.
                if ( cmd->source()->isLocal() )
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
                        qDebug() << "Setting lastop for source" << cmd->source()->id() << "to" << cmd->guid();

                        TomahawkSqlQuery query = m_dbimpl->newquery();
                        query.prepare( "UPDATE source SET lastop = ? WHERE id = ?" );
                        query.addBindValue( cmd->guid() );
                        query.addBindValue( cmd->source()->id() );

                        if( !query.exec() )
                        {
                            qDebug() << "Failed to set lastop";
                            throw "Failed to set lastop";
                        }
                    }
                }
            }

            if ( cmd->doesMutates() )
            {
                qDebug() << "Committing" << cmd->commandname();;
                if( !m_dbimpl->database().commit() )
                {

                    qDebug() << "*FAILED TO COMMIT TRANSACTION*";
                    throw "commit failed";
                }
                else
                {
                    qDebug() << "Committed" << cmd->commandname();
                }
            }

            //uint duration = timer.elapsed();
            //qDebug() << "DBCmd Duration:" << duration << "ms, now running postcommit for" << cmd->commandname();
            cmd->postCommit();
            //qDebug() << "Post commit finished for"<<  cmd->commandname();
        }
    }
    catch( const char * msg )
    {
        qDebug() << endl
                 << "*ERROR* processing databasecommand:"
                 << cmd->commandname()
                 << msg
                 << m_dbimpl->database().lastError().databaseText()
                 << m_dbimpl->database().lastError().driverText()
                 << endl;

        if( cmd->doesMutates() )
            m_dbimpl->database().rollback();

//        Q_ASSERT( false );
    }
    catch(...)
    {
        qDebug() << "Uncaught exception processing dbcmd";
        if( cmd->doesMutates() )
            m_dbimpl->database().rollback();

        Q_ASSERT( false );
        throw;
    }

    cmd->emitFinished();

    QMutexLocker lock( &m_mut );
    m_outstanding--;
    if ( m_outstanding > 0 )
        QTimer::singleShot( 0, this, SLOT( doWork() ) );
}


// this should take a const command, need to check/make json stuff mutable for some objs tho maybe.
void
DatabaseWorker::logOp( DatabaseCommandLoggable* command )
{
    TomahawkSqlQuery oplogquery = m_dbimpl->newquery();
    oplogquery.prepare( "INSERT INTO oplog(source, guid, command, singleton, compressed, json) "
                        "VALUES(?, ?, ?, ?, ?, ?)" );

    QVariantMap variant = QJson::QObjectHelper::qobject2qvariant( command );
    QByteArray ba = m_serializer.serialize( variant );

//     qDebug() << "OP JSON:" << ba.isNull() << ba << "from:" << variant; // debug

    bool compressed = false;
    if( ba.length() >= 512 )
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
        qDebug() << "Singleton command, deleting previous oplog commands";

        TomahawkSqlQuery oplogdelquery = m_dbimpl->newquery();
        oplogdelquery.prepare( QString( "DELETE FROM oplog WHERE source %1 AND singleton = 'true' AND command = ?" )
                                  .arg( command->source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( command->source()->id() ) ) );

        oplogdelquery.bindValue( 0, command->commandname() );
        oplogdelquery.exec();
    }

    qDebug() << "Saving to oplog:" << command->commandname()
             << "bytes:" << ba.length()
             << "guid:" << command->guid();

    oplogquery.bindValue( 0, command->source()->isLocal() ?
                          QVariant(QVariant::Int) : command->source()->id() );
    oplogquery.bindValue( 1, command->guid() );
    oplogquery.bindValue( 2, command->commandname() );
    oplogquery.bindValue( 3, command->singletonCmd() );
    oplogquery.bindValue( 4, compressed );
    oplogquery.bindValue( 5, ba );
    if( !oplogquery.exec() )
    {
        qDebug() << "Error saving to oplog";
        throw "Failed to save to oplog";
    }
}
