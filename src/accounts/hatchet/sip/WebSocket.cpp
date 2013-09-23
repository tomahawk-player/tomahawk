/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012,      Leo Franchi <lfranchi@kde.org>
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
#include "WebSocket.h"

#include "utils/Logger.h"

#include <functional>

typedef typename websocketpp::lib::error_code error_code;

WebSocket::WebSocket( const QString& url )
    : QObject( nullptr )
    , m_disconnecting( false )
    , m_url( url )
    , m_outputStream()
    , m_lastSocketState( QAbstractSocket::UnconnectedState )
    , m_connectionTimer( this )
{
    tLog() << Q_FUNC_INFO << "WebSocket constructing";
    m_client = std::unique_ptr< hatchet_client >( new hatchet_client() );
    m_client->set_message_handler( std::bind(&onMessage, this, std::placeholders::_1, std::placeholders::_2 ) );
    m_client->set_close_handler( std::bind(&onClose, this, std::placeholders::_1 ) );
    m_client->register_ostream( &m_outputStream );

    m_connectionTimer.setSingleShot( true );
    m_connectionTimer.setInterval( 30000 );
    connect( &m_connectionTimer, SIGNAL( timeout() ), SLOT( disconnectWs() ) );
}


WebSocket::~WebSocket()
{
    if ( m_connection )
        m_connection.reset();

    m_client.reset();

    if ( m_socket )
        delete m_socket.data();
}


void
WebSocket::setUrl( const QString &url )
{
    tLog() << Q_FUNC_INFO << "Setting url to" << url;
    if ( m_url == url )
        return;

    // We'll let automatic reconnection handle things
    if ( m_socket && m_socket->isEncrypted() )
        disconnectWs();
}


void
WebSocket::connectWs()
{
    tLog() << Q_FUNC_INFO << "Connecting";
    m_disconnecting = false;
    if ( m_socket )
    {
        if ( m_socket->isEncrypted() )
            return;

        if ( m_socket->state() == QAbstractSocket::ClosingState )
            QMetaObject::invokeMethod( this, "connectWs", Qt::QueuedConnection );

        return;
    }

    tLog() << Q_FUNC_INFO << "Establishing new connection";
    m_socket = QPointer< QSslSocket >( new QSslSocket( nullptr ) );
    m_socket->addCaCertificate( QSslCertificate::fromPath( ":/hatchet-account/startcomroot.pem").first() );
    QObject::connect( m_socket, SIGNAL( stateChanged( QAbstractSocket::SocketState ) ), SLOT( socketStateChanged( QAbstractSocket::SocketState ) ) );
    QObject::connect( m_socket, SIGNAL( sslErrors( const QList< QSslError >& ) ), SLOT( sslErrors( const QList< QSslError >& ) ) );
    QObject::connect( m_socket, SIGNAL( encrypted() ), SLOT( encrypted() ) );
    QObject::connect( m_socket, SIGNAL( readyRead() ), SLOT( socketReadyRead() ) );
    m_socket->connectToHostEncrypted( m_url.host(), m_url.port() );
    m_connectionTimer.start();
}


void
WebSocket::disconnectWs( websocketpp::close::status::value status, const QString &reason )
{
    tLog() << Q_FUNC_INFO << "Disconnecting";
    m_disconnecting = true;

    error_code ec;
    if ( m_connection )
    {
        m_connection->close( status, reason.toLatin1().constData(), ec );
        QMetaObject::invokeMethod( this, "readOutput", Qt::QueuedConnection );
        QTimer::singleShot( 5000, this, SLOT( disconnectSocket() ) ); //safety
        return;
    }

    disconnectSocket();
}


void
WebSocket::disconnectSocket()
{
    if ( m_socket && m_socket->state() == QAbstractSocket::ConnectedState )
        m_socket->disconnectFromHost();
    else
        QMetaObject::invokeMethod( this, "cleanup", Qt::QueuedConnection );

    QTimer::singleShot( 5000, this, SLOT( cleanup() ) ); //safety
}


void
WebSocket::cleanup()
{
    tLog() << Q_FUNC_INFO << "Cleaning up";
    m_outputStream.seekg( std::ios_base::end );
    m_outputStream.seekp( std::ios_base::end );
    m_queuedMessagesToSend.empty();
    if ( m_connection )
        m_connection.reset();

    emit disconnected();
}


void
WebSocket::socketStateChanged( QAbstractSocket::SocketState state )
{
    tLog() << Q_FUNC_INFO << "Socket state changed to" << state;
    switch ( state )
    {
        case QAbstractSocket::ClosingState:
            if ( m_lastSocketState == QAbstractSocket::ClosingState )
            {
                // It seems like it does not actually properly close, so force it
                tLog() << Q_FUNC_INFO << "Got a double closing state, cleaning up and emitting disconnected";
                m_socket->deleteLater();
                m_lastSocketState = QAbstractSocket::UnconnectedState;
                QMetaObject::invokeMethod( this, "cleanup", Qt::QueuedConnection );
                return;
            }
            break;
        case QAbstractSocket::UnconnectedState:
            if ( m_lastSocketState == QAbstractSocket::UnconnectedState )
                return;
            tLog() << Q_FUNC_INFO << "Socket now unconnected, cleaning up and emitting disconnected";
            m_socket->deleteLater();
            m_lastSocketState = QAbstractSocket::UnconnectedState;
            QMetaObject::invokeMethod( this, "cleanup", Qt::QueuedConnection );
            return;
        default:
            ;
    }
    m_lastSocketState = state;
}


void
WebSocket::sslErrors( const QList< QSslError >& errors )
{
    tLog() << Q_FUNC_INFO << "Encountered errors when trying to connect via SSL";
    foreach( QSslError error, errors )
        tLog() << Q_FUNC_INFO << "Error: " << error.errorString();
    QMetaObject::invokeMethod( this, "disconnectWs", Qt::QueuedConnection );
}


void
WebSocket::encrypted()
{
    tLog() << Q_FUNC_INFO << "Encrypted connection to Dreamcatcher established";
    error_code ec;

    QUrl url(m_url);
    websocketpp::uri_ptr uri(new websocketpp::uri(false, url.host().toStdString(), url.port(), "/"));

    m_connection = m_client->get_connection( uri, ec );
    if ( !m_connection || ec.value() != 0 )
    {
        tLog() << Q_FUNC_INFO << "Got error creating WS connection, error is:" << QString::fromStdString( ec.message() );
        disconnectWs();
        return;
    }
    m_client->connect( m_connection );
    QMetaObject::invokeMethod( this, "readOutput", Qt::QueuedConnection );
    emit connected();
}


void
WebSocket::readOutput()
{
    if ( !m_connection )
        return;

    std::string outputString = m_outputStream.str();
    if ( outputString.size() > 0 )
    {
        m_outputStream.str("");

        qint64 sizeWritten = m_socket->write( outputString.data(), outputString.size() );
        tDebug() << Q_FUNC_INFO << "Got " << outputString.size() << "from outstream, wrote" << sizeWritten << "bytes to the socket";
        if ( sizeWritten == -1 )
        {
            tLog() << Q_FUNC_INFO << "Error during writing, closing connection";
            QMetaObject::invokeMethod( this, "disconnectWs", Qt::QueuedConnection );
            return;
        }
    }

    if ( m_queuedMessagesToSend.size() )
    {
        if ( m_connection->get_state() == websocketpp::session::state::open )
        {
            foreach( QByteArray message, m_queuedMessagesToSend )
            {
                tDebug() << Q_FUNC_INFO << "Sending queued message of size" << message.size();
                m_connection->send( std::string( message.constData(), message.size() ), websocketpp::frame::opcode::TEXT );
            }

            m_queuedMessagesToSend.clear();
            QMetaObject::invokeMethod( this, "readOutput", Qt::QueuedConnection );
            m_connectionTimer.stop();
        }
        else if ( !m_disconnecting )
            QTimer::singleShot( 200, this, SLOT( readOutput() ) );
    }
    else
        m_connectionTimer.stop();
}

void
WebSocket::socketReadyRead()
{
    if ( !m_socket || !m_socket->isEncrypted() )
        return;

    if ( !m_socket->isValid() )
    {
        tLog() << Q_FUNC_INFO << "Socket appears to no longer be valid. Something is wrong; disconnecting";
        QMetaObject::invokeMethod( this, "disconnectWs", Qt::QueuedConnection );
        return;
    }

    if ( qint64 bytes = m_socket->bytesAvailable() )
    {
        QByteArray buf;
        buf.resize( bytes );
        qint64 bytesRead = m_socket->read( buf.data(), bytes );
        tDebug() << Q_FUNC_INFO << "Bytes available: " << bytes << ", bytes read:" << bytesRead; // << ", content is" << websocketpp::utility::to_hex( buf.constData(), bytesRead ).data();
        if ( bytesRead != bytes )
        {
            tLog() << Q_FUNC_INFO << "Error occurred during socket read. Something is wrong; disconnecting";
            QMetaObject::invokeMethod( this, "disconnectWs", Qt::QueuedConnection );
            return;
        }
        std::stringstream ss( std::string( buf.constData(), bytesRead ) );
        ss >> *m_connection;
    }

    QMetaObject::invokeMethod( this, "readOutput", Qt::QueuedConnection );
}


void
WebSocket::encodeMessage( const QByteArray &bytes )
{
    if ( !m_connection )
    {
        tLog() << Q_FUNC_INFO << "Asked to send message but do not have a valid connection!";
        return;
    }

    if ( m_connection->get_state() != websocketpp::session::state::open )
    {
        tLog() << Q_FUNC_INFO << "Connection not yet open/upgraded, queueing work to send";
        m_queuedMessagesToSend.append( bytes );
        m_connectionTimer.start();
    }
    else
        m_connection->send( std::string( bytes.constData() ), websocketpp::frame::opcode::TEXT );

    QMetaObject::invokeMethod( this, "readOutput", Qt::QueuedConnection );
}

void
onMessage( WebSocket* ws, websocketpp::connection_hdl, hatchet_client::message_ptr msg )
{
    tDebug() << Q_FUNC_INFO << "Handling message";
    std::string payload = msg->get_payload();
    ws->decodedMessage( QByteArray( payload.data(), payload.length() ) );
}

void
onClose( WebSocket *ws, websocketpp::connection_hdl )
{
    tDebug() << Q_FUNC_INFO << "Handling message";
    QMetaObject::invokeMethod( ws, "disconnectSocket", Qt::QueuedConnection );
}
