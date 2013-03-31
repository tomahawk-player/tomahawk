/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "ControlConnection.h"

#include "StreamConnection.h"
#include "database/Database.h"
#include "database/DatabaseCommand_CollectionStats.h"
#include "DbSyncConnection.h"
#include "SourceList.h"
#include "network/DbSyncConnection.h"
#include "network/Servent.h"
#include "sip/PeerInfo.h"
#include "utils/Logger.h"

#define TCP_TIMEOUT 600

using namespace Tomahawk;


ControlConnection::ControlConnection( Servent* parent )
    : Connection( parent )
    , m_dbsyncconn( 0 )
    , m_registered( false )
    , m_pingtimer( 0 )
{
    qDebug() << "CTOR controlconnection";
    setId("ControlConnection()");

    // auto delete when connection closes:
    connect( this, SIGNAL( finished() ), SLOT( deleteLater() ) );

    this->setMsgProcessorModeIn( MsgProcessor::UNCOMPRESS_ALL | MsgProcessor::PARSE_JSON );
    this->setMsgProcessorModeOut( MsgProcessor::COMPRESS_IF_LARGE );
}


ControlConnection::~ControlConnection()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << id() << name();

    if ( !m_source.isNull() )
        m_source->setOffline();

    delete m_pingtimer;
    m_servent->unregisterControlConnection( this );
    if ( m_dbsyncconn )
        m_dbsyncconn->deleteLater();
}


source_ptr
ControlConnection::source() const
{
    return m_source;
}


Connection*
ControlConnection::clone()
{
    ControlConnection* clone = new ControlConnection( servent() );
    clone->setOnceOnly( onceOnly() );
    clone->setName( name() );
    return clone;
}


void
ControlConnection::setup()
{
    qDebug() << Q_FUNC_INFO << id() << name();

    if ( !m_source.isNull() )
    {
        qDebug() << "This source seems to be online already.";
        Q_ASSERT( false );
        return;
    }

    QString friendlyName = name();

    tDebug() << "Detected name:" << name() << friendlyName << m_sock->peerAddress();

    // setup source and remote collection for this peer
    m_source = SourceList::instance()->get( id(), friendlyName, true );
    m_source->setControlConnection( this );

    // delay setting up collection/etc until source is synced.
    // we need it DB synced so it has an ID + exists in DB.
    connect( m_source.data(), SIGNAL( syncedWithDatabase() ),
                                SLOT( registerSource() ), Qt::QueuedConnection );

    m_source->setOnline();

    m_pingtimer = new QTimer;
    m_pingtimer->setInterval( 5000 );
    connect( m_pingtimer, SIGNAL( timeout() ), SLOT( onPingTimer() ) );
    m_pingtimer->start();
    m_pingtimer_mark.start();
}


// source was synced to DB, set it up properly:
void
ControlConnection::registerSource()
{
    qDebug() << Q_FUNC_INFO << m_source->id();
    Source* source = (Source*) sender();
    Q_UNUSED( source )
    Q_ASSERT( source == m_source.data() );

    m_registered = true;
    setupDbSyncConnection();
}


void
ControlConnection::setupDbSyncConnection( bool ondemand )
{
    qDebug() << Q_FUNC_INFO << ondemand << m_source->id() << m_dbconnkey << m_dbsyncconn << m_registered;

    if ( m_dbsyncconn || !m_registered )
        return;

    Q_ASSERT( m_source->id() > 0 );

    if ( !m_dbconnkey.isEmpty() )
    {
        qDebug() << "Connecting to DBSync offer from peer...";
        m_dbsyncconn = new DBSyncConnection( m_servent, m_source );

        m_servent->createParallelConnection( this, m_dbsyncconn, m_dbconnkey );
        m_dbconnkey.clear();
    }
    else if ( !outbound() || ondemand ) // only one end makes the offer
    {
        qDebug() << "Offering a DBSync key to peer...";
        m_dbsyncconn = new DBSyncConnection( m_servent, m_source );

        QString key = uuid();
        m_servent->registerOffer( key, m_dbsyncconn );
        QVariantMap m;
        m.insert( "method", "dbsync-offer" );
        m.insert( "key", key );
        sendMsg( m );
    }

    if ( m_dbsyncconn )
    {
        connect( m_dbsyncconn, SIGNAL( finished() ),
                 m_dbsyncconn,   SLOT( deleteLater() ) );

        connect( m_dbsyncconn, SIGNAL( destroyed( QObject* ) ),
                                 SLOT( dbSyncConnFinished( QObject* ) ), Qt::DirectConnection );
    }
}


void
ControlConnection::dbSyncConnFinished( QObject* c )
{
    qDebug() << Q_FUNC_INFO << "DBSync connection closed (for now)";
    if ( (DBSyncConnection*)c == m_dbsyncconn )
    {
        //qDebug() << "Setting m_dbsyncconn to NULL";
        m_dbsyncconn = NULL;
    }
    else
        qDebug() << "Old DbSyncConn destroyed?!";
}


DBSyncConnection*
ControlConnection::dbSyncConnection()
{
    qDebug() << Q_FUNC_INFO << m_source->id();
    if ( !m_dbsyncconn )
    {
        setupDbSyncConnection( true );
//        Q_ASSERT( m_dbsyncconn );
    }

    return m_dbsyncconn;
}


void
ControlConnection::handleMsg( msg_ptr msg )
{
    if ( msg->is( Msg::PING ) )
    {
        // qDebug() << "Received Connection PING, nice." << m_pingtimer_mark.elapsed();
        m_pingtimer_mark.restart();
        return;
    }

    // if small and not compresed, print it out for debug
    if ( msg->length() < 1024 && !msg->is( Msg::COMPRESSED ) )
    {
        qDebug() << id() << "got msg:" << QString::fromLatin1( msg->payload() );
    }

    // All control connection msgs are JSON
    if ( !msg->is( Msg::JSON ) )
    {
        Q_ASSERT( msg->is( Msg::JSON ) );
        markAsFailed();
        return;
    }

    QVariantMap m = msg->json().toMap();
    if ( !m.isEmpty() )
    {
        if ( m.value( "conntype" ).toString() == "request-offer" )
        {
            QString theirkey = m["key"].toString();
            QString ourkey   = m["offer"].toString();
            QString theirdbid = m["controlid"].toString();
            servent()->reverseOfferRequest( this, theirdbid, ourkey, theirkey );
        }
        else if ( m.value( "method" ).toString() == "dbsync-offer" )
        {
            m_dbconnkey = m.value( "key" ).toString() ;
            setupDbSyncConnection();
        }
        else if ( m.value( "method" ) == "protovercheckfail" )
        {
            qDebug() << "*** Remote peer protocol version mismatch, connection closed";
            shutdown( true );
            return;
        }
        else
        {
            tDebug() << id() << "Unhandled msg:" << QString::fromLatin1( msg->payload() );
        }

        return;
    }

    tDebug() << id() << "Invalid msg:" << QString::fromLatin1( msg->payload() );
}


void
ControlConnection::onPingTimer()
{
    if ( m_pingtimer_mark.elapsed() >= TCP_TIMEOUT * 1000 )
    {
        qDebug() << "Timeout reached! Shutting down connection to" << m_source->friendlyName();
        shutdown( true );
    }

    sendMsg( Msg::factory( QByteArray(), Msg::PING ) );
}


void
ControlConnection::addPeerInfo( const peerinfo_ptr& peerInfo )
{
    peerInfo->setControlConnection( this );
    m_peerInfos.insert( peerInfo );
}


void
ControlConnection::removePeerInfo( const peerinfo_ptr& peerInfo )
{
    peerInfoDebug( peerInfo ) << "Remove peer from control connection:" << name();

    Q_ASSERT( peerInfo->controlConnection() == this );
//     TODO: find out why this happens
//     Q_ASSERT( m_peerInfos.contains( peerInfo ) );

    m_peerInfos.remove( peerInfo );

    if ( m_peerInfos.isEmpty() )
    {
        shutdown( true );
    }
}


const QSet< peerinfo_ptr >
ControlConnection::peerInfos() const
{
    return m_peerInfos;
}
