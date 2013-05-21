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
#include "WebSocketThreadController.h"
#include "WebSocket.h"

#include "utils/Logger.h"

WebSocketThreadController::WebSocketThreadController( QObject* sip )
    : QThread( nullptr )
    , m_sip( sip )
{
}


WebSocketThreadController::~WebSocketThreadController()
{
    if ( m_webSocket )
    {
        delete m_webSocket;
        m_webSocket = 0;
    }
}


void
WebSocketThreadController::setUrl( const QString &url )
{
    m_url = url;
    if ( m_webSocket )
    {
        QMetaObject::invokeMethod( m_webSocket, "setUrl", Qt::QueuedConnection, Q_ARG( QString, url ));
    }
}


void
WebSocketThreadController::run()
{
    tLog() << Q_FUNC_INFO << "Starting";
    m_webSocket = QPointer< WebSocket >( new WebSocket( m_url ) );
    if ( m_webSocket && m_sip )
    {
        tLog() << Q_FUNC_INFO << "Have a valid websocket and parent";
        connect( m_sip, SIGNAL( connectWebSocket() ), m_webSocket, SLOT( connectWs() ), Qt::QueuedConnection );
        connect( m_sip, SIGNAL( disconnectWebSocket() ), m_webSocket, SLOT( disconnectWs() ), Qt::QueuedConnection );
        connect( m_sip, SIGNAL( rawBytes( QByteArray ) ), m_webSocket, SLOT( encodeMessage( QByteArray ) ), Qt::QueuedConnection );
        connect( m_webSocket, SIGNAL( connected() ), m_sip, SLOT( webSocketConnected() ), Qt::QueuedConnection );
        connect( m_webSocket, SIGNAL( disconnected() ), m_sip, SLOT( webSocketDisconnected() ), Qt::QueuedConnection );
        connect( m_webSocket, SIGNAL( decodedMessage( QByteArray ) ), m_sip, SLOT( messageReceived( QByteArray ) ), Qt::QueuedConnection );
        QMetaObject::invokeMethod( m_webSocket, "connectWs", Qt::QueuedConnection );
        exec();
        delete m_webSocket;
        m_webSocket = 0;
    }
}
