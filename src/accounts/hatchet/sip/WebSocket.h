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
#ifndef WEBSOCKET__H
#define WEBSOCKET__H

#include "hatchet_config.hpp"
#include <websocketpp/client.hpp>

#include <QPointer>
#include <QSslSocket>
#include <QTimer>
#include <QUrl>

#include <memory>

typedef typename websocketpp::client< websocketpp::config::hatchet_client > hatchet_client;

class WebSocket;

void onMessage( WebSocket* ws, websocketpp::connection_hdl, hatchet_client::message_ptr msg );
void onClose( WebSocket* ws, websocketpp::connection_hdl );

class WebSocket : public QObject
{
    Q_OBJECT
public:
    explicit WebSocket( const QString& url );
    virtual ~WebSocket();

signals:
    void connected();
    void disconnected();
    void decodedMessage( QByteArray bytes );

public slots:
    void setUrl( const QString& url );
    void connectWs();
    void disconnectWs( websocketpp::close::status::value status = websocketpp::close::status::normal, const QString& reason = QString( "Disconnecting" ) );
    void encodeMessage( const QByteArray& bytes );

private slots:
    void socketStateChanged( QAbstractSocket::SocketState state );
    void sslErrors( const QList< QSslError >& errors );
    void disconnectSocket();
    void cleanup();
    void encrypted();
    void readOutput();
    void socketReadyRead();

private:
    Q_DISABLE_COPY( WebSocket )

    friend void onMessage( WebSocket *ws, websocketpp::connection_hdl, hatchet_client::message_ptr msg );
    friend void onClose( WebSocket *ws, websocketpp::connection_hdl );

    bool m_disconnecting;
    QUrl m_url;
    std::stringstream m_outputStream;
    std::unique_ptr< hatchet_client > m_client;
    hatchet_client::connection_ptr m_connection;
    QPointer< QSslSocket > m_socket;
    QAbstractSocket::SocketState m_lastSocketState;
    QList< QByteArray > m_queuedMessagesToSend;
    QTimer m_connectionTimer;
};

#endif
