/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "Connection_p.h"

#include "QTcpSocketExtra.h"
#include "network/Servent.h"
#include "utils/Logger.h"
#include "Source.h"

#include <QtCore/QTime>
#include <QtCore/QThread>

#define PROTOVER "4" // must match remote peer, or we can't talk.


Connection::Connection( Servent* parent )
    : QObject()
    , m_sock( 0 )
    , m_servent( parent )
    , d_ptr( new ConnectionPrivate( this ) )
{
    moveToThread( m_servent->thread() );
    tDebug( LOGVERBOSE ) << "CTOR Connection (super)" << thread();

    connect( &d_func()->msgprocessor_out, SIGNAL( ready( msg_ptr ) ),
             SLOT( sendMsg_now( msg_ptr ) ), Qt::QueuedConnection );

    connect( &d_func()->msgprocessor_in,  SIGNAL( ready( msg_ptr ) ),
             SLOT( handleMsg( msg_ptr ) ), Qt::QueuedConnection );

    connect( &d_func()->msgprocessor_in, SIGNAL( empty() ),
             SLOT( handleIncomingQueueEmpty() ), Qt::QueuedConnection );
}


Connection::~Connection()
{
    tDebug( LOGVERBOSE ) << "DTOR connection (super)" << id() << thread() << m_sock.isNull();
    if ( !m_sock.isNull() )
    {
        m_sock->deleteLater();
    }

    delete d_func()->statstimer;
    delete d_ptr;
}


void
Connection::handleIncomingQueueEmpty()
{
    //qDebug() << Q_FUNC_INFO << "bavail" << m_sock->bytesAvailable()
    //         << "isopen" << m_sock->isOpen()
    //         << "m_peer_disconnected" << m_peer_disconnected
    //         << "bytes rx" << bytesReceived();

    if ( !m_sock.isNull() && m_sock->bytesAvailable() == 0 && d_func()->peer_disconnected )
    {
        tDebug( LOGVERBOSE ) << "No more data to read, peer disconnected. shutting down connection."
                             << "bytesavail" << m_sock->bytesAvailable()
                             << "bytesrx" << d_func()->rx_bytes;
        shutdown();
    }
}


// convenience:
void
Connection::setFirstMessage( const QVariant& m )
{
    QJson::Serializer ser;
    const QByteArray ba = ser.serialize( m );
    //qDebug() << "first msg json len:" << ba.length();
    setFirstMessage( Msg::factory( ba, Msg::JSON ) );
}


void
Connection::setFirstMessage( msg_ptr m )
{
    Q_D( Connection );

    d->firstmsg = m;
    //qDebug() << id() << " first msg set to " << QString::fromAscii(m_firstmsg->payload())
    //        << "msg len:" << m_firstmsg->length() ;
}

msg_ptr
Connection::firstMessage() const
{
    Q_D( const Connection );

    return d->firstmsg;
}

const QPointer<QTcpSocket>&
Connection::socket()
{
    return m_sock;
}

void
Connection::setOutbound( bool o )
{
    Q_D( Connection );

    d->outbound = o;
}

bool
Connection::outbound() const
{
    Q_D( const Connection );

    return d->outbound;
}

Servent*
Connection::servent() const
{
    return m_servent;
}

int
Connection::peerPort() const
{
    Q_D( const Connection );

    return d->peerport;
}

void
Connection::setPeerPort(int p)
{
    Q_D( Connection );

    d->peerport = p;
}


void
Connection::shutdown( bool waitUntilSentAll )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << waitUntilSentAll << id();
    if ( d_func()->do_shutdown )
    {
        //qDebug() << id() << " already shutting down";
        return;
    }

    d_func()->do_shutdown = true;
    if ( !waitUntilSentAll )
    {
//        qDebug() << "Shutting down immediately " << id();
        actualShutdown();
    }
    else
    {
        tDebug( LOGVERBOSE ) << "Shutting down after transfer complete " << id()
                             << "Actual/Desired" << d_func()->tx_bytes << d_func()->tx_bytes_requested;

        bytesWritten( 0 ); // trigger shutdown if we've already sent everything
        // otherwise the bytesWritten slot will call actualShutdown()
        // once all enqueued data has been properly written to the socket
    }
}


void
Connection::actualShutdown()
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << d_func()->actually_shutting_down << id();
    if ( d_func()->actually_shutting_down )
    {
        return;
    }
    d_func()->actually_shutting_down = true;

    if ( !m_sock.isNull() && m_sock->isOpen() )
    {
        m_sock->disconnectFromHost();
    }

//    qDebug() << "EMITTING finished()";
    emit finished();
}


void
Connection::markAsFailed()
{
    tDebug( LOGVERBOSE ) << "Connection" << id() << "FAILED ***************" << thread();
    emit failed();
    shutdown();
}

void
Connection::setName( const QString& n )
{
    Q_D( Connection );

    d->name = n;
}

QString
Connection::name() const
{
    Q_D( const Connection );

    return d->name;
}

void
Connection::setOnceOnly( bool b )
{
    Q_D( Connection );

    d->onceonly = b;
}

bool
Connection::onceOnly() const
{
    Q_D( const Connection );

    return d->onceonly;
}

bool
Connection::isReady() const
{
    Q_D( const Connection );

    return d->ready;
}

bool
Connection::isRunning() const
{
    return m_sock != 0;
}

qint64
Connection::bytesSent() const
{
    return d_func()->tx_bytes;
}

qint64
Connection::bytesReceived() const
{
    return d_func()->rx_bytes;
}

void
Connection::setMsgProcessorModeOut(quint32 m)
{
    d_func()->msgprocessor_out.setMode( m );
}

void
Connection::setMsgProcessorModeIn(quint32 m)
{
    d_func()->msgprocessor_in.setMode( m );
}

const QHostAddress
Connection::peerIpAddress() const
{
    return d_func()->peerIpAddress;
}


void
Connection::start( QTcpSocket* sock )
{
    Q_D( Connection );
    Q_ASSERT( m_sock.isNull() );
    Q_ASSERT( sock );
    Q_ASSERT( sock->isValid() );

    m_sock = sock;

    if ( d->name.isEmpty() )
    {
        d->name = QString( "peer[%1]" ).arg( m_sock->peerAddress().toString() );
    }

    QTimer::singleShot( 0, this, SLOT( checkACL() ) );
}


void
Connection::checkACL()
{
    if ( d_func()->nodeid.isEmpty() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Not checking ACL, nodeid is empty";
        QTimer::singleShot( 0, this, SLOT( doSetup() ) );
        return;
    }

    if ( Servent::isIPWhitelisted( d_func()->peerIpAddress ) )
    {
        QTimer::singleShot( 0, this, SLOT( doSetup() ) );
        return;
    }

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Checking ACL for" << name();
    connect( ACLRegistry::instance(), SIGNAL( aclResult( QString, QString, Tomahawk::ACLStatus::Type ) ),
             this, SLOT( checkACLResult( QString, QString, Tomahawk::ACLStatus::Type ) ),
             Qt::QueuedConnection );
    QMetaObject::invokeMethod( ACLRegistry::instance(), "isAuthorizedUser", Qt::QueuedConnection, Q_ARG( QString, d_func()->nodeid ), Q_ARG( QString, bareName() ), Q_ARG( Tomahawk::ACLStatus::Type, Tomahawk::ACLStatus::NotFound ) );
}


QString
Connection::bareName() const
{
    return name().contains( '/' ) ? name().left( name().indexOf( "/" ) ) : name();
}

void
Connection::checkACLResult( const QString &nodeid, const QString &username, Tomahawk::ACLStatus::Type peerStatus )
{
    if ( nodeid != d_func()->nodeid )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << QString( "nodeid (%1) not ours (%2) for user %3" ).arg( nodeid ).arg( d_func()->nodeid ).arg( username );
        return;
    }
    if ( username != bareName() )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "username not our barename";
        return;
    }

    disconnect( ACLRegistry::instance(), SIGNAL( aclResult( QString, QString, Tomahawk::ACLStatus::Type ) ) );
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << QString( "ACL status for user %1 is" ).arg( username ) << peerStatus;
    if ( peerStatus == Tomahawk::ACLStatus::Stream )
    {
        QTimer::singleShot( 0, this, SLOT( doSetup() ) );
        return;
    }

    shutdown();
}


void
Connection::authCheckTimeout()
{
    Q_D( Connection );

    if ( d->ready )
        return;

    tDebug( LOGVERBOSE ) << "Closing connection, not authed in time.";
    shutdown();
}


void
Connection::doSetup()
{
    Q_D( Connection );

    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << thread();
    /*
        New connections can be created from other thread contexts, such as
        when AudioEngine calls getIODevice.. - we need to ensure that connections
        and their associated sockets are running in the same thread as the servent.

        HINT: export QT_FATAL_WARNINGS=1 helps to catch these kind of errors.
     */
    if ( QThread::currentThread() != m_servent->thread() )
    {
        // Connections should always be in the same thread as the servent.
        moveToThread( m_servent->thread() );
    }

    //stats timer calculates BW used by this connection
    d_func()->statstimer = new QTimer;
    d_func()->statstimer->moveToThread( this->thread() );
    d_func()->statstimer->setInterval( 1000 );
    connect( d_func()->statstimer, SIGNAL( timeout() ), SLOT( calcStats() ) );
    d_func()->statstimer->start();
    d_func()->statstimer_mark.start();

    m_sock->moveToThread( thread() );

    connect( m_sock.data(), SIGNAL( bytesWritten( qint64 ) ),
                              SLOT( bytesWritten( qint64 ) ), Qt::QueuedConnection );

    connect( m_sock.data(), SIGNAL( disconnected() ),
                              SLOT( socketDisconnected() ), Qt::QueuedConnection );

    connect( m_sock.data(), SIGNAL( error( QAbstractSocket::SocketError ) ),
                              SLOT( socketDisconnectedError( QAbstractSocket::SocketError ) ), Qt::QueuedConnection );

    connect( m_sock.data(), SIGNAL( readyRead() ),
                              SLOT( readyRead() ), Qt::QueuedConnection );

    // if connection not authed/setup fast enough, kill it:
    QTimer::singleShot( AUTH_TIMEOUT, this, SLOT( authCheckTimeout() ) );

    if ( outbound() )
    {
        Q_ASSERT( !d->firstmsg.isNull() );
        sendMsg( d->firstmsg );
    }
    else
    {
        sendMsg( Msg::factory( PROTOVER, Msg::SETUP ) );
    }

    // call readyRead incase we missed the signal in between the servent disconnecting and us
    // connecting to the signal - won't do anything if there are no bytesAvailable anyway.
    readyRead();
}


void
Connection::socketDisconnected()
{
    qint64 bytesAvailable = 0;
    if ( !m_sock.isNull() )
    {
        bytesAvailable = m_sock->bytesAvailable();
    }
    tDebug( LOGVERBOSE ) << "SOCKET DISCONNECTED" << this->name() << id()
                         << "shutdown will happen after incoming queue empties."
                         << "bytesavail:" << bytesAvailable
                         << "bytesRecvd" << bytesReceived();

    d_func()->peer_disconnected = true;
    emit socketClosed();

    if ( d_func()->msgprocessor_in.length() == 0 && bytesAvailable == 0 )
    {
        handleIncomingQueueEmpty();
        actualShutdown();
    }
}


void
Connection::socketDisconnectedError( QAbstractSocket::SocketError e )
{
    tDebug() << "SOCKET ERROR CODE" << e << this->name() << "CALLING Connection::shutdown(false)";

    if ( e == QAbstractSocket::RemoteHostClosedError )
        return;

    d_func()->peer_disconnected = true;

    emit socketErrored(e);
    emit socketClosed();

    shutdown( false );
}


QString
Connection::id() const
{
    return d_func()->id;
}


void
Connection::setId( const QString& id )
{
    d_func()->id = id;
}

QString
Connection::nodeId() const
{
    return d_func()->nodeid;
}

void
Connection::setNodeId( const QString& nodeid )
{
    d_func()->nodeid = nodeid;
}


void
Connection::readyRead()
{
//    qDebug() << "readyRead, bytesavail:" << m_sock->bytesAvailable();
    Q_D( Connection );

    if ( d->msg.isNull() )
    {
        if ( m_sock->bytesAvailable() < Msg::headerSize() )
            return;

        char msgheader[ Msg::headerSize() ];
        if ( m_sock->read( (char*) &msgheader, Msg::headerSize() ) != Msg::headerSize() )
        {
            tDebug() << "Failed reading msg header";
            this->markAsFailed();
            return;
        }

        d->msg = Msg::begin( (char*) &msgheader );
        d->rx_bytes += Msg::headerSize();
    }

    if ( m_sock->bytesAvailable() < d->msg->length() )
        return;

    QByteArray ba = m_sock->read( d->msg->length() );
    if ( ba.length() != (qint32)d->msg->length() )
    {
        tDebug() << "Failed to read full msg payload";
        this->markAsFailed();
        return;
    }
    d->msg->fill( ba );
    d->rx_bytes += ba.length();

    handleReadMsg(); // process m_msg and clear() it

    // since there is no explicit threading, use the event loop to schedule this:
    if ( m_sock->bytesAvailable() )
    {
        QTimer::singleShot( 0, this, SLOT( readyRead() ) );
    }
}


void
Connection::handleReadMsg()
{
    Q_D( Connection );

    if ( outbound() == false &&
        d->msg->is( Msg::SETUP ) &&
        d->msg->payload() == "ok" )
    {
        d->ready = true;
        tDebug( LOGVERBOSE ) << "Connection" << id() << "READY";
        setup();
        emit ready();
    }
    else if ( !d->ready &&
             outbound() &&
             d->msg->is( Msg::SETUP ) )
    {
        if ( d->msg->payload() == PROTOVER )
        {
            sendMsg( Msg::factory( "ok", Msg::SETUP ) );
            d->ready = true;
            tDebug( LOGVERBOSE ) << "Connection" << id() << "READY";
            setup();
            emit ready();
        }
        else
        {
            sendMsg( Msg::factory( "{\"method\":\"protovercheckfail\"}", Msg::JSON | Msg::SETUP ) );
            shutdown( true );
        }
    }
    else
    {
        d->msgprocessor_in.append( d->msg );
    }

    d->msg.clear();
}


void
Connection::sendMsg( QVariant j )
{
    Q_D( Connection );

    if ( d->do_shutdown )
        return;

    QJson::Serializer serializer;
    const QByteArray payload = serializer.serialize( j );
    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Sending to" << id() << ":" << payload;
    sendMsg( Msg::factory( payload, Msg::JSON ) );
}


void
Connection::sendMsg( msg_ptr msg )
{
    if ( d_func()->do_shutdown )
    {
        tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "SHUTTING DOWN, NOT SENDING msg flags:"
                             << (int)msg->flags() << "length:" << msg->length() << id();
        return;
    }

    d_func()->tx_bytes_requested += msg->length() + Msg::headerSize();
    d_func()->msgprocessor_out.append( msg );
}


void
Connection::sendMsg_now( msg_ptr msg )
{
    Q_ASSERT( QThread::currentThread() == thread() );
//    Q_ASSERT( this->isRunning() );

    if ( m_sock.isNull() || !m_sock->isOpen() || !m_sock->isWritable() )
    {
        tDebug() << "***** Socket problem, whilst in sendMsg(). Cleaning up. *****";
        shutdown( false );
        return;
    }

    if ( !msg->write( m_sock.data() ) )
    {
        //qDebug() << "Error writing to socket in sendMsg() *************";
        shutdown( false );
        return;
    }
}


void
Connection::bytesWritten( qint64 i )
{
    d_func()->tx_bytes += i;
    // if we are waiting to shutdown, and have sent all queued data, do actual shutdown:
    if ( d_func()->do_shutdown && d_func()->tx_bytes == d_func()->tx_bytes_requested )
        actualShutdown();
}


void
Connection::calcStats()
{
    int elapsed = d_func()->statstimer_mark.restart(); // ms since last calc

    d_func()->stats_tx_bytes_per_sec = (float)1000 * ( (d_func()->tx_bytes - d_func()->tx_bytes_last) / (float)elapsed );
    d_func()->stats_rx_bytes_per_sec = (float)1000 * ( (d_func()->rx_bytes - d_func()->rx_bytes_last) / (float)elapsed );

    d_func()->rx_bytes_last = d_func()->rx_bytes;
    d_func()->tx_bytes_last = d_func()->tx_bytes;

    emit statsTick( d_func()->stats_tx_bytes_per_sec, d_func()->stats_rx_bytes_per_sec );
}
