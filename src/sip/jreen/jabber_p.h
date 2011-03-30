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
    This is the Jabber client that the rest of the app sees
    Gloox stuff should NOT leak outside this class.
    We may replace jreen later, this interface should remain the same.
*/
#ifndef JABBER_P_H
#define JABBER_P_H

#include <QObject>
#include <QSharedPointer>
#include <QMap>
#include <QNetworkProxy>

#include <string>

#include <jreen/client.h>
#include <jreen/disco.h>
#include <jreen/message.h>
#include <jreen/messagesession.h>
#include <jreen/stanza.h>
#include <jreen/jreen.h>
#include <jreen/error.h>
#include <jreen/presence.h>
#include <jreen/vcard.h>
#include <jreen/abstractroster.h>


#if defined( WIN32 ) || defined( _WIN32 )
# include <windows.h>
#endif

#include "../sipdllmacro.h"
#include <jreen/connection.h>
#include <jreen/mucroom.h>

class SIPDLLEXPORT Jabber_p :
       public QObject
{
Q_OBJECT

public:
    explicit Jabber_p( const QString& jid, const QString& password, const QString& server = "", const int port = -1 );
    virtual ~Jabber_p();

    void setProxy( QNetworkProxy* proxy );

signals:
    void msgReceived( const QString&, const QString& ); //from, msg
    void peerOnline( const QString& );
    void peerOffline( const QString& );
    void connected();
    void disconnected();
    void jidChanged( const QString& );
    void authError( int, const QString& );

public slots:
    void sendMsg( const QString& to, const QString& msg );
    void broadcastMsg( const QString& msg );
    void addContact( const QString& jid, const QString& msg = QString() );
    void disconnect();

    void onDisconnect(Jreen::Client::DisconnectReason reason);
    void onConnect();

private slots:
    virtual void onNewPresence( const Jreen::Presence& presence );
    virtual void onNewMessage( const Jreen::Message& msg );
    virtual void onError( const Jreen::Connection::SocketError& e )
    {
        qDebug() << e;
    }
    virtual void onIQ( const Jreen::IQ &iq, int context );

private:
    bool presenceMeansOnline( Jreen::Presence::Type p );
    Jreen::Client *m_client;
    Jreen::MUCRoom *m_room;
    Jreen::SimpleRoster *m_roster;
    Jreen::JID m_jid;
    QMap<Jreen::Presence::Type, QString> m_presences;
    QMap<QString, Jreen::Presence::Type> m_peers;
    QString m_server;

    enum m_iqContext { RequestDisco };
};

#endif // JABBER_H
