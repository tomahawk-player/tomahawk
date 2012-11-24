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

/*
    Database syncing using the oplog table.
    =======================================
    Load the last GUID we applied for the peer, tell them it.
    In return, they send us all new ops since that guid.

    We then apply those new ops to our cache of their data

    Synced.

*/

#include "DbSyncConnection.h"

#include "database/Database.h"
#include "database/DatabaseCommand.h"
#include "database/DatabaseCommand_CollectionStats.h"
#include "database/DatabaseCommand_LoadOps.h"
#include "RemoteCollection.h"
#include "Source.h"
#include "SourceList.h"
#include "utils/Logger.h"

using namespace Tomahawk;


DBSyncConnection::DBSyncConnection( Servent* s, const source_ptr& src )
    : Connection( s )
    , m_fetchCount( 0 )
    , m_source( src )
    , m_state( UNKNOWN )
{
    qDebug() << Q_FUNC_INFO << src->id() << thread();

    connect( this,            SIGNAL( stateChanged( DBSyncConnection::State, DBSyncConnection::State, QString ) ),
             m_source.data(),   SLOT( onStateChanged( DBSyncConnection::State, DBSyncConnection::State, QString ) ) );
    connect( m_source.data(), SIGNAL( commandsFinished() ),
             this,              SLOT( lastOpApplied() ) );

    this->setMsgProcessorModeIn( MsgProcessor::PARSE_JSON | MsgProcessor::UNCOMPRESS_ALL );

    // msgs are stored compressed in the db, so not typically needed here, but doesnt hurt:
    this->setMsgProcessorModeOut( MsgProcessor::COMPRESS_IF_LARGE );
}


DBSyncConnection::~DBSyncConnection()
{
    tDebug() << "DTOR" << Q_FUNC_INFO << m_source->id() << m_source->friendlyName();
    m_state = SHUTDOWN;
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
    setId( QString( "DBSyncConnection/%1" ).arg( socket()->peerAddress().toString() ) );
    check();
}


void
DBSyncConnection::trigger()
{
    // if we're still setting up the connection, do nothing - we sync on first connect anyway:
    if ( !isRunning() )
        return;

    QMetaObject::invokeMethod( this, "sendMsg", Qt::QueuedConnection,
                               Q_ARG( msg_ptr, Msg::factory( "{\"method\":\"trigger\"}", Msg::JSON ) ) );
}


void
DBSyncConnection::check()
{
    qDebug() << Q_FUNC_INFO << this << m_source->id();

    if ( m_state == SHUTDOWN )
    {
        qDebug() << "Aborting sync due to shutdown.";
        return;
    }
    if ( m_state != UNKNOWN && m_state != SYNCED )
    {
        qDebug() << "Syncing in progress already.";
        return;
    }

    m_uscache.clear();
    changeState( CHECKING );

    if ( m_source->lastCmdGuid().isEmpty() )
    {
        tDebug( LOGVERBOSE ) << "Fetching lastCmdGuid from database!";
        DatabaseCommand_CollectionStats* cmd_them = new DatabaseCommand_CollectionStats( m_source );
        connect( cmd_them, SIGNAL( done( QVariantMap ) ), SLOT( gotThem( QVariantMap ) ) );
        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>(cmd_them) );
    }
    else
    {
        fetchOpsData( m_source->lastCmdGuid() );
    }
}


/// Called once we've loaded our cached data about their collection
void
DBSyncConnection::gotThem( const QVariantMap& m )
{
    fetchOpsData( m.value( "lastop" ).toString() );
}


void
DBSyncConnection::fetchOpsData( const QString& sinceguid )
{
    changeState( FETCHING );

    tLog() << "Sending a FETCHOPS cmd since:" << sinceguid << "- source:" << m_source->id();

    QVariantMap msg;
    msg.insert( "method", "fetchops" );
    msg.insert( "lastop", sinceguid );
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
        if ( cmd )
        {
            QSharedPointer<DatabaseCommand> cmdsp = QSharedPointer<DatabaseCommand>(cmd);
            m_source->addCommand( cmdsp );
        }

        if ( !msg->is( Msg::FRAGMENT ) ) // last msg in this batch
        {
            changeState( SAVING ); // just DB work left to complete
            m_source->executeCommands();
        }
        return;
    }

    if ( m.value( "method" ).toString() == "fetchops" )
    {
        ++m_fetchCount;
        tDebug( LOGVERBOSE ) << "Fetching new dbops:" << m["lastop"].toString() << m_fetchCount;
        m_uscache = m;
        sendOps();
        return;
    }

    if ( m.value( "method" ).toString() == "trigger" )
    {
        tLog() << "Got trigger msg on dbsyncconnection, checking for new stuff.";
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
    connect( cmd, SIGNAL( done( QString, QString, QList< dbop_ptr > ) ),
                    SLOT( sendOpsData( QString, QString, QList< dbop_ptr > ) ) );

    m_uscache.clear();

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
DBSyncConnection::sendOpsData( QString sinceguid, QString lastguid, QList< dbop_ptr > ops )
{
    if ( m_lastSentOp == lastguid )
        ops.clear();

    m_lastSentOp = lastguid;
    if ( ops.length() == 0 )
    {
        tLog( LOGVERBOSE ) << "Sending ok" << m_source->id() << m_source->friendlyName();
        sendMsg( Msg::factory( "ok", Msg::DBOP ) );
        return;
    }

    tLog( LOGVERBOSE ) << Q_FUNC_INFO << sinceguid << lastguid << "Num ops to send:" << ops.length();

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
