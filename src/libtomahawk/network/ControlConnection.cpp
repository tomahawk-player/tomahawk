/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include "ControlConnection_p.h"

#include "database/Database.h"
#include "database/DatabaseCommand_CollectionStats.h"
#include "network/DbSyncConnection.h"
#include "network/Msg.h"
#include "network/MsgProcessor.h"
#include "network/Servent.h"
#include "sip/PeerInfo.h"
#include "utils/Logger.h"

#include "PlaylistEntry.h"
#include "StreamConnection.h"
#include "SourceList.h"

#define TCP_TIMEOUT 600

using namespace Tomahawk;


ControlConnection::ControlConnection( Servent* parent )
    : Connection( parent )
    , d_ptr( new ControlConnectionPrivate( this ) )
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
    Q_D( ControlConnection );
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << id() << name();

    {
        QReadLocker locker( &d->sourceLock );
        if ( !d->source.isNull() )
        {
            d->source->setOffline();
        }
    }

    delete d->pingtimer;
    servent()->unregisterControlConnection( this );
    if ( d->dbsyncconn )
        d->dbsyncconn->deleteLater();
    delete d_ptr;
}


source_ptr
ControlConnection::source() const
{
    Q_D( const ControlConnection );
    // We return a copy of the shared pointer, no need for a longer lock
    QReadLocker locker( &d->sourceLock );
    return d->source;
}


void
ControlConnection::unbindFromSource()
{
    Q_D( ControlConnection );
    QWriteLocker locker( &d->sourceLock );
    d->source = Tomahawk::source_ptr();
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
    Q_D( ControlConnection );
    qDebug() << Q_FUNC_INFO << id() << name();
    // We need to manually lock, so that we can release before the end of the function
    d->sourceLock.lockForWrite();

    if ( !d->source.isNull() )
    {
        qDebug() << "This source seems to be online already.";
        Q_ASSERT( false );
        d->sourceLock.unlock();
        return;
    }

    QString friendlyName = name();

    tDebug() << "Detected name:" << name() << friendlyName;

    // setup source and remote collection for this peer
    d->source = SourceList::instance()->get( id(), friendlyName, true );
    QSharedPointer<QMutexLocker> locker = d->source->acquireLock();
    if ( d->source->setControlConnection( this ) )
    {
        // We are the new ControlConnection for this source

        // delay setting up collection/etc until source is synced.
        // we need it DB synced so it has an ID + exists in DB.
        connect( d->source.data(), SIGNAL( syncedWithDatabase() ),
                                    SLOT( registerSource() ), Qt::QueuedConnection );

        d->source->setOnline();

        d->pingtimer = new QTimer;
        d->pingtimer->setInterval( 5000 );
        connect( d->pingtimer, SIGNAL( timeout() ), SLOT( onPingTimer() ) );
        d->pingtimer->start();
        d->pingtimer_mark.start();
        d->sourceLock.unlock();
    }
    else
    {
        // We are not responsible for this source anymore, so do not keep a reference.
        d->source = Tomahawk::source_ptr();
        // Unlock before we delete ourselves
        d->sourceLock.unlock();
        // There is already another ControlConnection in use, we are useless.
        deleteLater();
    }
}


// source was synced to DB, set it up properly:
void
ControlConnection::registerSource()
{
    Q_D( ControlConnection );
    QReadLocker sourceLocker( &d->sourceLock );
    if ( d->source.isNull() )
    {
        // Not connected to a source anymore, nothing to do.
        return;
    }

    QSharedPointer<QMutexLocker> locker = d->source->acquireLock();
    // Only continue if we are still the ControlConnection associated with this source.
    if ( d->source->controlConnection() == this )
    {
        qDebug() << Q_FUNC_INFO << d->source->id();
        Source* source = (Source*) sender();
        Q_UNUSED( source )
        Q_ASSERT( source == d->source.data() );

        d->registered = true;
        setupDbSyncConnection();
    }
}


void
ControlConnection::setupDbSyncConnection( bool ondemand )
{
    Q_D( ControlConnection );
    QReadLocker locker( &d->sourceLock );
    if ( d->source.isNull() )
    {
        // We were unbind from the Source, nothing to do here, just waiting to be deleted.
        return;
    }

    qDebug() << Q_FUNC_INFO << ondemand << d->source->id() << d->dbconnkey << d->dbsyncconn << d->registered;

    if ( d->dbsyncconn || !d->registered )
        return;

    Q_ASSERT( d->source->id() > 0 );

    if ( !d->dbconnkey.isEmpty() )
    {
        qDebug() << "Connecting to DBSync offer from peer...";
        d->dbsyncconn = new DBSyncConnection( servent(), d->source );

        servent()->createParallelConnection( this, d->dbsyncconn, d->dbconnkey );
        d->dbconnkey.clear();
    }
    else if ( !outbound() || ondemand ) // only one end makes the offer
    {
        qDebug() << "Offering a DBSync key to peer...";
        d->dbsyncconn = new DBSyncConnection( servent(), d->source );

        QString key = uuid();
        servent()->registerOffer( key, d->dbsyncconn );
        QVariantMap m;
        m.insert( "method", "dbsync-offer" );
        m.insert( "key", key );
        sendMsg( m );
    }

    if ( d->dbsyncconn )
    {
        connect( d->dbsyncconn, SIGNAL( finished() ),
                 d->dbsyncconn,   SLOT( deleteLater() ) );

        connect( d->dbsyncconn, SIGNAL( destroyed( QObject* ) ),
                                 SLOT( dbSyncConnFinished( QObject* ) ), Qt::DirectConnection );
    }
}


void
ControlConnection::dbSyncConnFinished( QObject* c )
{
    Q_D( ControlConnection );
    qDebug() << Q_FUNC_INFO << "DBSync connection closed (for now)";
    if ( (DBSyncConnection*)c == d->dbsyncconn )
    {
        //qDebug() << "Setting m_dbsyncconn to NULL";
        d->dbsyncconn = NULL;
    }
    else
        qDebug() << "Old DbSyncConn destroyed?!";
}


DBSyncConnection*
ControlConnection::dbSyncConnection()
{
    Q_D( ControlConnection );
    if ( !d->dbsyncconn )
    {
        setupDbSyncConnection( true );
//        Q_ASSERT( m_dbsyncconn );
    }

    return d->dbsyncconn;
}


void
ControlConnection::handleMsg( msg_ptr msg )
{
    Q_D( ControlConnection );
    if ( msg->is( Msg::PING ) )
    {
        // qDebug() << "Received Connection PING, nice." << m_pingtimer_mark.elapsed();
        d->pingtimer_mark.restart();
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
        tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Received message was not in JSON format";
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
            d->dbconnkey = m.value( "key" ).toString() ;
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
ControlConnection::authCheckTimeout()
{
    if ( isReady() )
        return;

    Q_D( ControlConnection );
    Servent::instance()->queueForAclResult( bareName(), d->peerInfos );

    tDebug( LOGVERBOSE ) << "Closing connection, not authed in time.";
    shutdown();
}


void
ControlConnection::onPingTimer()
{
    Q_D( ControlConnection );
    if ( d->pingtimer_mark.elapsed() >= TCP_TIMEOUT * 1000 )
    {
        QReadLocker locker( &d->sourceLock );
        qDebug() << "Timeout reached! Shutting down connection to" << d->source->friendlyName();
        shutdown( true );
    }

    sendMsg( Msg::factory( QByteArray(), Msg::PING ) );
}


void
ControlConnection::addPeerInfo( const peerinfo_ptr& peerInfo )
{
    Q_D( ControlConnection );

    peerInfo->setControlConnection( this );
    d->peerInfos.insert( peerInfo );
}


void
ControlConnection::removePeerInfo( const peerinfo_ptr& peerInfo )
{
    Q_D( ControlConnection );
    peerInfoDebug( peerInfo ) << "Remove peer from control connection:" << name();

    Q_ASSERT( peerInfo->controlConnection() == this );
//     TODO: find out why this happens
//     Q_ASSERT( m_peerInfos.contains( peerInfo ) );

    d->peerInfos.remove( peerInfo );

    if ( d->peerInfos.isEmpty() && d->shutdownOnEmptyPeerInfos )
    {
        shutdown( true );
    }
}

void
ControlConnection::setShutdownOnEmptyPeerInfos( bool shutdownOnEmptyPeerInfos )
{
    Q_D( ControlConnection );
    d->shutdownOnEmptyPeerInfos = shutdownOnEmptyPeerInfos;
    if ( d->peerInfos.isEmpty() && d->shutdownOnEmptyPeerInfos )
    {
        shutdown( true );
    }
}


const QSet< peerinfo_ptr >
ControlConnection::peerInfos() const
{
    Q_D( const ControlConnection );
    return d->peerInfos;
}
