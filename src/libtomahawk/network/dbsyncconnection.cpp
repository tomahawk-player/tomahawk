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

/*
    Database syncing using the oplog table.
    =======================================
    Load the last GUID we applied for the peer, tell them it.
    In return, they send us all new ops since that guid.

    We then apply those new ops to our cache of their data

    Synced.

*/

#include "dbsyncconnection.h"

#include "database/database.h"
#include "database/databasecommand.h"
#include "database/databasecommand_collectionstats.h"
#include "database/databasecommand_loadops.h"
#include "remotecollection.h"
#include "source.h"
#include "sourcelist.h"
#include "utils/logger.h"

// close the dbsync connection after this much inactivity.
// it's automatically reestablished as needed.
#define IDLE_TIMEOUT 300000

using namespace Tomahawk;


DBSyncConnection::DBSyncConnection( Servent* s, source_ptr src )
    : Connection( s )
    , m_source( src )
    , m_state( UNKNOWN )
{
    qDebug() << Q_FUNC_INFO << src->id() << thread();
    connect( this,            SIGNAL( stateChanged( DBSyncConnection::State, DBSyncConnection::State, QString ) ),
             m_source.data(),   SLOT( onStateChanged( DBSyncConnection::State, DBSyncConnection::State, QString ) ) );

    m_timer.setInterval( IDLE_TIMEOUT );
    connect( &m_timer, SIGNAL( timeout() ), SLOT( idleTimeout() ) );

    this->setMsgProcessorModeIn( MsgProcessor::PARSE_JSON | MsgProcessor::UNCOMPRESS_ALL );

    // msgs are stored compressed in the db, so not typically needed here, but doesnt hurt:
    this->setMsgProcessorModeOut( MsgProcessor::COMPRESS_IF_LARGE );
}


DBSyncConnection::~DBSyncConnection()
{
    qDebug() << "DTOR" << Q_FUNC_INFO;
    m_state = SHUTDOWN;
}


void
DBSyncConnection::idleTimeout()
{
    qDebug() << Q_FUNC_INFO;
    shutdown( true );
}


void
DBSyncConnection::changeState( State newstate )
{
    if ( m_state == SHUTDOWN )
        return;

    State s = m_state;
    m_state = newstate;
    qDebug() << "DBSYNC State changed from" << s << "to" << newstate << "- source:" << m_source->id();
    emit stateChanged( newstate, s, "" );
}


void
DBSyncConnection::setup()
{
//    qDebug() << Q_FUNC_INFO;
    setId( QString( "DBSyncConnection/%1" ).arg( socket()->peerAddress().toString() ) );
    check();
}


void
DBSyncConnection::trigger()
{
//    qDebug() << Q_FUNC_INFO;

    // if we're still setting up the connection, do nothing - we sync on first connect anyway:
    if ( !isRunning() )
        return;

    QMetaObject::invokeMethod( this, "sendMsg", Qt::QueuedConnection,
                               Q_ARG( msg_ptr,
                                      Msg::factory( "{\"method\":\"trigger\"}", Msg::JSON ) )
                             );
}


void
DBSyncConnection::check()
{
    qDebug() << Q_FUNC_INFO << m_source->id();
    if ( m_state != UNKNOWN && m_state != SYNCED )
    {
        qDebug() << "Syncing in progress already.";
        return;
    }
    if ( m_state == SHUTDOWN )
    {
        qDebug() << "Aborting sync due to shutdown.";
        return;
    }

    m_uscache.clear();
    m_themcache.clear();
    m_us.clear();

    changeState( CHECKING );

    // load last-modified etc data for our collection and theirs from our DB:
    DatabaseCommand_CollectionStats* cmd_us =
            new DatabaseCommand_CollectionStats( SourceList::instance()->getLocal() );

    DatabaseCommand_CollectionStats* cmd_them =
            new DatabaseCommand_CollectionStats( m_source );

    connect( cmd_us, SIGNAL( done( QVariantMap ) ),
                       SLOT( gotUs( QVariantMap ) ) );

    connect( cmd_them, SIGNAL( done( QVariantMap ) ),
                         SLOT( gotThemCache( QVariantMap ) ) );


    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd_us) );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd_them) );

    // restarts idle countdown
    m_timer.start();
}


/// Called once we've loaded our mtimes etc from the DB for our local
/// collection - send them to the remote peer to compare.
void
DBSyncConnection::gotUs( const QVariantMap& m )
{
    m_us = m;
    if ( !m_uscache.empty() )
        sendOps();
}


/// Called once we've loaded our cached data about their collection
void
DBSyncConnection::gotThemCache( const QVariantMap& m )
{
    m_themcache = m;
    changeState( FETCHING );

    tLog() << "Sending a FETCHOPS cmd since:" << m_themcache.value( "lastop" ).toString();

    QVariantMap msg;
    msg.insert( "method", "fetchops" );
    msg.insert( "lastop", m_themcache.value( "lastop" ).toString() );
    sendMsg( msg );
}


void
DBSyncConnection::handleMsg( msg_ptr msg )
{
    Q_ASSERT( !msg->is( Msg::COMPRESSED ) );

    if ( m_state == FETCHING )
        changeState( PARSING );

    // "everything is synced" indicated by non-json msg containing "ok":
    if ( !msg->is( Msg::JSON ) &&
         msg->is( Msg::DBOP ) &&
         msg->payload() == "ok" )
    {
//        qDebug() << "No ops to apply, we are synced.";
        changeState( SYNCED );
        // calc the collection stats, to updates the "X tracks" in the sidebar etc
        // this is done automatically if you run a dbcmd to add files.
        DatabaseCommand_CollectionStats* cmd = new DatabaseCommand_CollectionStats( m_source );
        connect( cmd,           SIGNAL( done( const QVariantMap & ) ),
                 m_source.data(), SLOT( setStats( const QVariantMap& ) ), Qt::QueuedConnection );
        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd) );
        return;
    }

    Q_ASSERT( msg->is( Msg::JSON ) );

    QVariantMap m = msg->json().toMap();
    if ( m.empty() )
    {
        tLog() << "Failed to parse msg in dbsync" << m_source->id() << m_source->friendlyName();
        Q_ASSERT( false );
        return;
    }

    // a db sync op msg
    if ( msg->is( Msg::DBOP ) )
    {
        DatabaseCommand* cmd = DatabaseCommand::factory( m, m_source );
        if ( !cmd )
        {
            qDebug() << "UNKNOWN DBOP CMD";

            if ( !msg->is( Msg::FRAGMENT ) ) // last msg in this batch
                lastOpApplied();
            return;
        }

//        qDebug() << "APPLYING CMD" << cmd->commandname() << cmd->guid();

        if ( !msg->is( Msg::FRAGMENT ) ) // last msg in this batch
        {
            changeState( SAVING ); // just DB work left to complete
            connect( cmd, SIGNAL( finished() ), SLOT( lastOpApplied() ) );
        }

        if ( !cmd->singletonCmd() )
            m_source->setLastOpGuid( cmd->guid() );

        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
        return;
    }

    if ( m.value( "method" ).toString() == "fetchops" )
    {
        m_uscache = m;
        if ( !m_us.empty() )
            sendOps();
        return;
    }

    if ( m.value( "method" ).toString() == "trigger" )
    {
//        qDebug() << "Got trigger msg on dbsyncconnection, checking for new stuff.";
        check();
        return;
    }

    tLog() << Q_FUNC_INFO << "Unhandled msg:" << msg->payload();
    Q_ASSERT( false );
}


void
DBSyncConnection::lastOpApplied()
{
    changeState( SYNCED );
    // check again, until peer responds we have no new ops to process
    check();
}


/// request new copies of anything we've cached that is stale
void
DBSyncConnection::sendOps()
{
    tLog() << "Will send peer" << m_source->id() << "all ops since" << m_uscache.value( "lastop" ).toString();

    source_ptr src = SourceList::instance()->getLocal();

    DatabaseCommand_loadOps* cmd = new DatabaseCommand_loadOps( src, m_uscache.value( "lastop" ).toString() );
    connect( cmd,  SIGNAL( done( QString, QString, QList< dbop_ptr > ) ),
                     SLOT( sendOpsData( QString, QString, QList< dbop_ptr > ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
DBSyncConnection::sendOpsData( QString sinceguid, QString lastguid, QList< dbop_ptr > ops )
{
    if ( m_lastSentOp == lastguid )
        ops.clear();

    qDebug() << Q_FUNC_INFO << sinceguid << lastguid << "Num ops to send:" << ops.length();
    m_lastSentOp = lastguid;
    if ( ops.length() == 0 )
    {
        sendMsg( Msg::factory( "ok", Msg::DBOP ) );
        return;
    }

    int i;
    for( i = 0; i < ops.length(); ++i )
    {
        quint8 flags = Msg::JSON | Msg::DBOP;

        if ( ops.at( i )->compressed )
            flags |= Msg::COMPRESSED;
        if ( i != ops.length() - 1 )
            flags |= Msg::FRAGMENT;

        sendMsg( Msg::factory( ops.at( i )->payload, flags ) );
    }
}


Connection*
DBSyncConnection::clone()
{
    Q_ASSERT( false );
    return 0;
}
