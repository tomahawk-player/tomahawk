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

#include "Database.h"

#include "utils/Json.h"
#include "utils/Logger.h"

#include "DatabaseCommand.h"
#include "DatabaseImpl.h"
#include "DatabaseWorker.h"
#include "IdThreadWorker.h"
#include "PlaylistEntry.h"

#include "DatabaseCommand_AddFiles.h"
#include "DatabaseCommand_CreatePlaylist.h"
#include "DatabaseCommand_DeleteFiles.h"
#include "DatabaseCommand_DeletePlaylist.h"
#include "DatabaseCommand_LogPlayback.h"
#include "DatabaseCommand_RenamePlaylist.h"
#include "DatabaseCommand_SetPlaylistRevision.h"
#include "DatabaseCommand_CreateDynamicPlaylist.h"
#include "DatabaseCommand_DeleteDynamicPlaylist.h"
#include "DatabaseCommand_SetDynamicPlaylistRevision.h"
#include "DatabaseCommand_SocialAction.h"
#include "DatabaseCommand_ShareTrack.h"
#include "DatabaseCommand_SetCollectionAttributes.h"
#include "DatabaseCommand_SetTrackAttributes.h"

#define DEFAULT_WORKER_THREADS 4
#define MAX_WORKER_THREADS 16

namespace Tomahawk
{

dbcmd_ptr
DatabaseCommandFactory::newInstance()
{
    dbcmd_ptr command = dbcmd_ptr( create() );

    notifyCreated( command );

    return command;
}


void
DatabaseCommandFactory::notifyCreated( const dbcmd_ptr& command )
{
    command->setWeakRef( command.toWeakRef() );

    emit created( command );
}


Database* Database::s_instance = 0;


Database*
Database::instance()
{
    return s_instance;
}


Database::Database( const QString& dbname, QObject* parent )
    : QObject( parent )
    , m_ready( false )
    , m_impl( new DatabaseImpl( dbname ) )
    , m_workerRW( new DatabaseWorkerThread( this, true ) )
    , m_idWorker( new IdThreadWorker( this ) )
{
    s_instance = this;

    // register commands
    registerCommand<DatabaseCommand_AddFiles>();
    registerCommand<DatabaseCommand_DeleteFiles>();
    registerCommand<DatabaseCommand_CreatePlaylist>();
    registerCommand<DatabaseCommand_DeletePlaylist>();
    registerCommand<DatabaseCommand_LogPlayback>();
    registerCommand<DatabaseCommand_RenamePlaylist>();
    registerCommand<DatabaseCommand_SetPlaylistRevision>();
    registerCommand<DatabaseCommand_CreateDynamicPlaylist>();
    registerCommand<DatabaseCommand_DeleteDynamicPlaylist>();
    registerCommand<DatabaseCommand_SetDynamicPlaylistRevision>();
    registerCommand<DatabaseCommand_SocialAction>();
    registerCommand<DatabaseCommand_SetCollectionAttributes>();
    registerCommand<DatabaseCommand_SetTrackAttributes>();
    registerCommand<DatabaseCommand_ShareTrack>();

    if ( MAX_WORKER_THREADS < DEFAULT_WORKER_THREADS )
        m_maxConcurrentThreads = MAX_WORKER_THREADS;
    else
        m_maxConcurrentThreads = qBound( DEFAULT_WORKER_THREADS, QThread::idealThreadCount(), MAX_WORKER_THREADS );

    tDebug() << Q_FUNC_INFO << "Using" << m_maxConcurrentThreads << "database worker threads";

    connect( m_impl, SIGNAL( indexReady() ), SLOT( markAsReady() ) );
    connect( m_impl, SIGNAL( indexStarted() ), SIGNAL( indexStarted() ) );
    connect( m_impl, SIGNAL( indexReady() ), SIGNAL( indexReady() ) );

    Q_ASSERT( m_workerRW );
    m_workerRW.data()->start();

    while ( m_workerThreads.count() < m_maxConcurrentThreads )
    {
        QPointer< DatabaseWorkerThread > workerThread( new DatabaseWorkerThread( this, false ) );
        Q_ASSERT( workerThread );
        workerThread.data()->start();
        m_workerThreads << workerThread;
    }
    m_idWorker->start();
}


Database::~Database()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO;

    m_idWorker->stop();
    delete m_idWorker;

    if ( m_workerRW )
    {
        // Ensure event loop was started so quit() has any effect.
        m_workerRW->waitForEventLoopStart();
        m_workerRW.data()->quit();
    }
    foreach ( QPointer< DatabaseWorkerThread > workerThread, m_workerThreads )
    {
        // Ensure event loop was started so quit() has any effect.
        workerThread->waitForEventLoopStart();
        // If event loop already was killed, the following is just a no-op.
        workerThread->quit();
    }

    emit waitingForWorkers();
    if ( m_workerRW )
    {
        m_workerRW.data()->wait();
        delete m_workerRW.data();
    }
    foreach ( QPointer< DatabaseWorkerThread > workerThread, m_workerThreads )
    {
        if ( workerThread )
        {
            workerThread.data()->wait();
            delete workerThread.data();
        }
    }
    m_workerThreads.clear();

    qDeleteAll( m_implHash.values() );
    qDeleteAll( m_commandFactories.values() );
    delete m_impl;

    emit workersFinished();
}


void
Database::loadIndex()
{
    m_impl->loadIndex();
}


void
Database::enqueue( const QList< Tomahawk::dbcmd_ptr >& lc )
{
    Q_ASSERT( m_ready );
    if ( !m_ready )
    {
        tDebug() << "Can't enqueue DatabaseCommand, Database is not ready yet!";
        return;
    }

    foreach ( const Tomahawk::dbcmd_ptr& cmd, lc )
    {
        DatabaseCommandFactory* factory = commandFactoryByCommandName( cmd->commandname() );
        if ( factory )
        {
            factory->notifyCreated( cmd );
        }
    }

    tDebug( LOGVERBOSE ) << "Enqueueing" << lc.count() << "commands to rw thread";
    if ( m_workerRW && m_workerRW.data()->worker() )
        m_workerRW.data()->worker().data()->enqueue( lc );
}


void
Database::enqueue( const Tomahawk::dbcmd_ptr& lc )
{
    Q_ASSERT( m_ready );
    if ( !m_ready )
    {
        tDebug() << "Can't enqueue DatabaseCommand, Database is not ready yet!";
        return;
    }

    DatabaseCommandFactory* factory = commandFactoryByCommandName( lc->commandname() );
    if ( factory )
    {
        factory->notifyCreated( lc );
    }

    if ( lc->doesMutates() )
    {
        tDebug( LOGVERBOSE ) << "Enqueueing command to rw thread:" << lc->commandname();
        if ( m_workerRW && m_workerRW.data()->worker() )
            m_workerRW.data()->worker().data()->enqueue( lc );
    }
    else
    {
        // find thread for commandname with lowest amount of outstanding jobs and enqueue job
        int busyThreads = 0;
        QPointer< DatabaseWorkerThread > workerThread;
        QPointer< DatabaseWorker > happyWorker;
        for ( int i = 0; i < m_workerThreads.count(); i++ )
        {
            workerThread = m_workerThreads.at( i );

            if ( !workerThread || !workerThread->worker() )
            {
                // We have no valid worker for the current thread so skip it.
                continue;
            }

            if ( !workerThread->worker()->busy() )
            {
                // Case 1: We have a workerThread with no outstanding jobs.
                // As this is the optimal situation we do not need to look at
                // the other workers.
                happyWorker = workerThread->worker();
                break;
            }
            else
            {
                busyThreads++;

                if ( !happyWorker )
                {
                    // Case 2: We have not yet got a happyWorker but the current
                    // workerThread has a worker so use it as a fallback.
                    happyWorker = workerThread->worker();
                }
                else if ( workerThread->worker()->outstandingJobs() < happyWorker->outstandingJobs() )
                {
                    // Case 3: We have a worker and the current worker is less busy
                    // than the previous minimum.
                    happyWorker = workerThread->worker();
                }
            }
        }

        tDebug( LOGVERBOSE ) << "Enqueueing command to thread:" << happyWorker << busyThreads << lc->commandname();
        Q_ASSERT( happyWorker );
        happyWorker->enqueue( lc );
    }
}


DatabaseImpl*
Database::impl()
{
    QMutexLocker lock( &m_mutex );

    QThread* thread = QThread::currentThread();
    if ( !m_implHash.contains( thread ) )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Creating database impl for thread" << QThread::currentThread();
        DatabaseImpl* impl = m_impl->clone();
        m_implHash.insert( thread, impl );
    }

    return m_implHash.value( thread );
}


void
Database::markAsReady()
{
    if ( m_ready )
        return;

    tLog() << Q_FUNC_INFO << "Database is ready now!";

    // In addition to a ready index, we also need at leat one workerThread to
    // be ready so that we can queue DatabaseCommands.
    if ( m_workerThreads.size() > 0 && m_workerThreads.first() )
    {
        m_workerThreads.first()->waitForEventLoopStart();
    }

    m_ready = true;
    emit ready();
}


void
Database::registerCommand( DatabaseCommandFactory* commandFactory )
{
    // this is ugly, but we don't have virtual static methods in C++ :(
    dbcmd_ptr command = commandFactory->newInstance();

    const QString commandName = command->commandname();
    const QString className = command->metaObject()->className();

    tDebug( LOGVERBOSE ) << "Registering command" << commandName << "from class" << className;

    if( m_commandFactories.keys().contains( commandName ) )
    {
        tLog() << commandName << "is already in " << m_commandFactories.keys();
    }
    Q_ASSERT( !m_commandFactories.keys().contains( commandName ) );

    m_commandNameClassNameMapping.insert( commandName, className );
    m_commandFactories.insert( commandName, commandFactory );
}


DatabaseCommandFactory*
Database::commandFactoryByClassName(const QString& className)
{
    const QString commandName = m_commandNameClassNameMapping.key( className );
    return commandFactoryByCommandName( commandName );
}


DatabaseCommandFactory*
Database::commandFactoryByCommandName(const QString& commandName )
{
    return m_commandFactories.value( commandName );
}



dbcmd_ptr
Database::createCommandInstance( const QString& commandName )
{
    DatabaseCommandFactory* factory = commandFactoryByCommandName( commandName );

    if( !factory )
    {
         tLog() << "Unknown database command" << commandName;
         return dbcmd_ptr();
    }

    return factory->newInstance();
}


dbcmd_ptr
Database::createCommandInstance(const QVariant& op, const source_ptr& source)
{
    const QString commandName = op.toMap().value( "command" ).toString();

    dbcmd_ptr command = createCommandInstance( commandName );
    if ( command.isNull() )
        return command;

    command->setSource( source );
    TomahawkUtils::qvariant2qobject( op.toMap(), command.data() );
    return command;
}

}

