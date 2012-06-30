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

#include "Connection.h"

#include "network/Servent.h"
#include "utils/Logger.h"
#include "Source.h"

#include <QtCore/QTime>
#include <QtCore/QThread>

#define PROTOVER "4" // must match remote peer, or we can't talk.


Connection::Connection( Servent* parent )
    : QObject()
    , m_sock( 0 )
    , m_peerport( 0 )
    , m_servent( parent )
    , m_ready( false )
    , m_onceonly( true )
    , m_do_shutdown( false )
    , m_actually_shutting_down( false )
    , m_peer_disconnected( false )
    , m_tx_bytes( 0 )
    , m_tx_bytes_requested( 0 )
    , m_rx_bytes( 0 )
    , m_id( "Connection()" )
    , m_statstimer( 0 )
    , m_stats_tx_bytes_per_sec( 0 )
    , m_stats_rx_bytes_per_sec( 0 )
    , m_rx_bytes_last( 0 )
    , m_tx_bytes_last( 0 )
{
    moveToThread( m_servent->thread() );
    qDebug() << "CTOR Connection (super)" << thread();

    connect( &m_msgprocessor_out, SIGNAL( ready( msg_ptr ) ),
             SLOT( sendMsg_now( msg_ptr ) ), Qt::QueuedConnection );

    connect( &m_msgprocessor_in,  SIGNAL( ready( msg_ptr ) ),
             SLOT( handleMsg( msg_ptr ) ), Qt::QueuedConnection );

    connect( &m_msgprocessor_in, SIGNAL( empty() ),
             SLOT( handleIncomingQueueEmpty() ), Qt::QueuedConnection );
}


Connection::~Connection()
{
    tDebug() << "DTOR connection (super)" << id() << thread() << m_sock.isNull();
    if( !m_sock.isNull() )
    {
//        qDebug() << "deleteLatering sock" << m_sock;
        m_sock->deleteLater();
    }

    delete m_statstimer;
}


void
Connection::handleIncomingQueueEmpty()
{
    //qDebug() << Q_FUNC_INFO << "bavail" << m_sock->bytesAvailable()
    //         << "isopen" << m_sock->isOpen()
    //         << "m_peer_disconnected" << m_peer_disconnected
    //         << "bytes rx" << bytesReceived();

    if( !m_sock.isNull() && m_sock->bytesAvailable() == 0 && m_peer_disconnected )
    {
        qDebug() << "No more data to read, peer disconnected. shutting down connection."
                 << "bytesavail" << m_sock->bytesAvailable()
                 << "bytesrx" << m_rx_bytes;
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
    m_firstmsg = m;
    //qDebug() << id() << " first msg set to " << QString::fromAscii(m_firstmsg->payload())
    //        << "msg len:" << m_firstmsg->length() ;
}


void
Connection::shutdown( bool waitUntilSentAll )
{
    qDebug() << Q_FUNC_INFO << waitUntilSentAll << id();
    if ( m_do_shutdown )
    {
        //qDebug() << id() << " already shutting down";
        return;
    }

    m_do_shutdown = true;
    if ( !waitUntilSentAll )
    {
//        qDebug() << "Shutting down immediately " << id();
        actualShutdown();
    }
    else
    {
        qDebug() << "Shutting down after transfer complete " << id()
                 << "Actual/Desired" << m_tx_bytes << m_tx_bytes_requested;

        bytesWritten( 0 ); // trigger shutdown if we've already sent everything
        // otherwise the bytesWritten slot will call actualShutdown()
        // once all enqueued data has been properly written to the socket
    }
}


void
Connection::actualShutdown()
{
    qDebug() << Q_FUNC_INFO << m_actually_shutting_down << id();
    if ( m_actually_shutting_down )
    {
        return;
    }
    m_actually_shutting_down = true;

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
    qDebug() << "Connection" << id() << "FAILED ***************" << thread();
    emit failed();
    shutdown();
}


void
Connection::start( QTcpSocket* sock )
{
    Q_ASSERT( m_sock.isNull() );
    Q_ASSERT( sock );
    Q_ASSERT( sock->isValid() );

    m_sock = sock;

    if( m_name.isEmpty() )
    {
        m_name = QString( "peer[%1]" ).arg( m_sock->peerAddress().toString() );
    }

    QTimer::singleShot( 0, this, SLOT( checkACL() ) );
}


void
Connection::checkACL()
{
    if ( !property( "nodeid" ).isValid() )
    {
        tLog() << Q_FUNC_INFO << "Not checking ACL, nodeid is empty";
        QTimer::singleShot( 0, this, SLOT( doSetup() ) );
        return;
    }

    if ( Servent::isIPWhitelisted( m_peerIpAddress ) )
    {
        QTimer::singleShot( 0, this, SLOT( doSetup() ) );
        return;
    }

    QString nodeid = property( "nodeid" ).toString();
    QString bareName = name().contains( '/' ) ? name().left( name().indexOf( "/" ) ) : name();
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Checking ACL for" << name();
    connect( ACLRegistry::instance(), SIGNAL( aclResult( QString, QString, ACLRegistry::ACL ) ), this, SLOT( checkACLResult( QString, QString, ACLRegistry::ACL ) ), Qt::QueuedConnection );
    QMetaObject::invokeMethod( ACLRegistry::instance(), "isAuthorizedUser", Qt::QueuedConnection, Q_ARG( QString, nodeid ), Q_ARG( QString, bareName ), Q_ARG( ACLRegistry::ACL, ACLRegistry::NotFound ) );
}


void
Connection::checkACLResult( const QString &nodeid, const QString &username, ACLRegistry::ACL peerStatus )
{
    QString bareName = name().contains( '/' ) ? name().left( name().indexOf( "/" ) ) : name();
    if ( nodeid != property( "nodeid" ).toString() || username != bareName )
    {
        tLog() << Q_FUNC_INFO << "nodeid not ours, or username not our barename";
        return;
    }

    disconnect( ACLRegistry::instance(), SIGNAL( aclResult( QString, QString, ACLRegistry::ACL ) ) );
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "ACL status is" << peerStatus;
    if ( peerStatus == ACLRegistry::Stream )
    {
        QTimer::singleShot( 0, this, SLOT( doSetup() ) );
        return;
    }

    shutdown();
}


void
Connection::authCheckTimeout()
{
    if( m_ready )
        return;

    qDebug() << "Closing connection, not authed in time.";
    shutdown();
}


void
Connection::doSetup()
{
    qDebug() << Q_FUNC_INFO << thread();
    /*
        New connections can be created from other thread contexts, such as
        when AudioEngine calls getIODevice.. - we need to ensure that connections
        and their associated sockets are running in the same thread as the servent.

        HINT: export QT_FATAL_WARNINGS=1 helps to catch these kind of errors.
     */
    if( QThread::currentThread() != m_servent->thread() )
    {
        // Connections should always be in the same thread as the servent.
        qDebug() << "Fixing thead affinity...";
        moveToThread( m_servent->thread() );
        qDebug() << Q_FUNC_INFO  << thread();
    }

    //stats timer calculates BW used by this connection
    m_statstimer = new QTimer;
    m_statstimer->moveToThread( this->thread() );
    m_statstimer->setInterval( 1000 );
    connect( m_statstimer, SIGNAL( timeout() ), SLOT( calcStats() ) );
    m_statstimer->start();
    m_statstimer_mark.start();

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

    if( outbound() )
    {
        Q_ASSERT( !m_firstmsg.isNull() );
        sendMsg( m_firstmsg );
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
    tDebug() << "SOCKET DISCONNECTED" << this->name() << id()
             << "shutdown will happen after incoming queue empties."
             << "bytesavail:" << m_sock->bytesAvailable()
             << "bytesRecvd" << bytesReceived();

    m_peer_disconnected = true;
    emit socketClosed();

    if( m_msgprocessor_in.length() == 0 && m_sock->bytesAvailable() == 0 )
    {
        handleIncomingQueueEmpty();
        actualShutdown();
    }
}


void
Connection::socketDisconnectedError( QAbstractSocket::SocketError e )
{
    qDebug() << "SOCKET ERROR CODE" << e << this->name() << "CALLING Connection::shutdown(false)";

    if ( e == QAbstractSocket::RemoteHostClosedError )
        return;

    m_peer_disconnected = true;

    emit socketErrored(e);
    emit socketClosed();

    shutdown( false );
}


QString
Connection::id() const
{
    return m_id;
}


void
Connection::setId( const QString& id )
{
    m_id = id;
}


void
Connection::readyRead()
{
//    qDebug() << "readyRead, bytesavail:" << m_sock->bytesAvailable();

    if( m_msg.isNull() )
    {
        if( m_sock->bytesAvailable() < Msg::headerSize() )
            return;

        char msgheader[ Msg::headerSize() ];
        if( m_sock->read( (char*) &msgheader, Msg::headerSize() ) != Msg::headerSize() )
        {
            qDebug() << "Failed reading msg header";
            this->markAsFailed();
            return;
        }

        m_msg = Msg::begin( (char*) &msgheader );
        m_rx_bytes += Msg::headerSize();
    }

    if( m_sock->bytesAvailable() < m_msg->length() )
        return;

    QByteArray ba = m_sock->read( m_msg->length() );
    if( ba.length() != (qint32)m_msg->length() )
    {
        qDebug() << "Failed to read full msg payload";
        this->markAsFailed();
        return;
    }
    m_msg->fill( ba );
    m_rx_bytes += ba.length();

    handleReadMsg(); // process m_msg and clear() it

    // since there is no explicit threading, use the event loop to schedule this:
    if( m_sock->bytesAvailable() )
    {
        QTimer::singleShot( 0, this, SLOT( readyRead() ) );
    }
}


void
Connection::handleReadMsg()
{
    if( outbound() == false &&
        m_msg->is( Msg::SETUP ) &&
        m_msg->payload() == "ok" )
    {
        m_ready = true;
        qDebug() << "Connection" << id() << "READY";
        setup();
        emit ready();
    }
    else if( !m_ready &&
             outbound() &&
             m_msg->is( Msg::SETUP ) )
    {
        if( m_msg->payload() == PROTOVER )
        {
            sendMsg( Msg::factory( "ok", Msg::SETUP ) );
            m_ready = true;
            qDebug() << "Connection" << id() << "READY";
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
        m_msgprocessor_in.append( m_msg );
    }

    m_msg.clear();
}


void
Connection::sendMsg( QVariant j )
{
    if( m_do_shutdown )
        return;

    QJson::Serializer serializer;
    const QByteArray payload = serializer.serialize( j );
    tLog( LOGVERBOSE ) << Q_FUNC_INFO << "Sending to" << id() << ":" << payload;
    sendMsg( Msg::factory( payload, Msg::JSON ) );
}


void
Connection::sendMsg( msg_ptr msg )
{
    if( m_do_shutdown )
    {
        qDebug() << Q_FUNC_INFO << "SHUTTING DOWN, NOT SENDING msg flags:"
                << (int)msg->flags() << "length:" << msg->length() << id();
        return;
    }

    m_tx_bytes_requested += msg->length() + Msg::headerSize();
    m_msgprocessor_out.append( msg );
}


void
Connection::sendMsg_now( msg_ptr msg )
{
    //qDebug() << Q_FUNC_INFO << thread() << QThread::currentThread();
    Q_ASSERT( QThread::currentThread() == thread() );
//    Q_ASSERT( this->isRunning() );

    if ( m_sock.isNull() || !m_sock->isOpen() || !m_sock->isWritable() )
    {
        qDebug() << "***** Socket problem, whilst in sendMsg(). Cleaning up. *****";
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
    m_tx_bytes += i;
    // if we are waiting to shutdown, and have sent all queued data, do actual shutdown:
    if ( m_do_shutdown && m_tx_bytes == m_tx_bytes_requested )
        actualShutdown();
}


void
Connection::calcStats()
{
    int elapsed = m_statstimer_mark.restart(); // ms since last calc

    m_stats_tx_bytes_per_sec = (float)1000 * ( (m_tx_bytes - m_tx_bytes_last) / (float)elapsed );
    m_stats_rx_bytes_per_sec = (float)1000 * ( (m_rx_bytes - m_rx_bytes_last) / (float)elapsed );

    m_rx_bytes_last = m_rx_bytes;
    m_tx_bytes_last = m_tx_bytes;

    emit statsTick( m_stats_tx_bytes_per_sec, m_stats_rx_bytes_per_sec );
}
