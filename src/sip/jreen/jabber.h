/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Dominik Schmidt <dev@dominik-schmidt.de>
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

#ifndef JABBER_H
#define JABBER_H

#include "sip/SipPlugin.h"

#include "avatarmanager.h"

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
#include <jreen/connection.h>
#include <jreen/mucroom.h>

#include <QNetworkProxy>
#include <QMessageBox>

#define MYNAME "SIPJREEN"
#define TOMAHAWK_FEATURE QLatin1String( "tomahawk:sip:v1" )
#define TOMAHAWK_CAP_NODE_NAME QLatin1String( "http://tomahawk-player.org/" )

#include "../sipdllmacro.h"

class SIPDLLEXPORT JabberPlugin : public SipPlugin
{
    Q_OBJECT
    Q_INTERFACES( SipPlugin )

public:
    JabberPlugin();
    virtual ~JabberPlugin();

    //FIXME: Make this more correct
    virtual bool isValid() { return true; }
    virtual const QString name();
    virtual const QString friendlyName();
    virtual const QString accountName();
    virtual QMenu* menu();

signals:
    void jidChanged( const QString& );

public slots:
    virtual bool connectPlugin( bool startup );
    void disconnectPlugin();
    void checkSettings();
    void sendMsg( const QString& to, const QString& msg );
    void broadcastMsg( const QString &msg );
    void addContact( const QString &jid, const QString& msg = QString() );
    void setProxy( const QNetworkProxy &proxy );

private slots:
    void showAddFriendDialog();
    void onConnect();
    void onDisconnect(Jreen::Client::DisconnectReason reason);
    void onAuthError(int code, const QString &msg);

    void onPresenceReceived( const Jreen::RosterItem::Ptr &item, const Jreen::Presence& presence );
    void onSubscriptionReceived( const Jreen::RosterItem::Ptr &item, const Jreen::Presence& presence );
    void onSubscriptionRequestConfirmed( int result );

    void onNewMessage( const Jreen::Message& message );
    void onError( const Jreen::Connection::SocketError& e )
    {
        qDebug() << e;
    }
    void onNewIq( const Jreen::IQ &iq, int context = NoContext );
    void onNewAvatar( const QString &jid );

private:
    void setupClientHelper();
    void addMenuHelper();
    void removeMenuHelper();

    bool presenceMeansOnline( Jreen::Presence::Type p );
    void handlePeerStatus( const Jreen::JID &jid, Jreen::Presence::Type presenceType );

    bool m_connected;

    QMenu* m_menu;
    QAction* m_addFriendAction;

    QString m_currentUsername;
    QString m_currentPassword;
    QString m_currentServer;
    unsigned int m_currentPort;
    QString m_currentResource;

    // sort out
    Jreen::Client *m_client;

    Jreen::MUCRoom *m_room;
    Jreen::SimpleRoster *m_roster;
    QHash<Jreen::JID, Jreen::Presence::Type> m_peers;
    QHash<Jreen::JID, QMessageBox*> m_subscriptionConfirmBoxes;
    enum IqContext { NoContext, RequestDisco, RequestedDisco, SipMessageSent, RequestedVCard };
    QStringList m_legacy_peers;
    AvatarManager *m_avatarManager;
};

#endif
