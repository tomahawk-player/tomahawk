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

#ifndef XMPPSIP_H
#define XMPPSIP_H

#include "sip/SipPlugin.h"

#include "avatarmanager.h"
#include "xmlconsole.h"

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

#include <QMessageBox>

#define TOMAHAWK_FEATURE QLatin1String( "tomahawk:sip:v1" )
#define TOMAHAWK_CAP_NODE_NAME QLatin1String( "http://tomahawk-player.org/" )

#include "../sipdllmacro.h"

class SIPDLLEXPORT XmppSipPlugin : public SipPlugin
{
    Q_OBJECT

public:
    XmppSipPlugin( Tomahawk::Accounts::Account* account );
    virtual ~XmppSipPlugin();

    //FIXME: Make this more correct
    virtual bool isValid() const { return true; }
    virtual ConnectionState connectionState() const;
    virtual QMenu* menu();

signals:
    void jidChanged( const QString& );

public slots:
    virtual bool connectPlugin();
    void disconnectPlugin();
    void checkSettings();
    void configurationChanged();
    void sendMsg( const QString& to, const QString& msg );
    void broadcastMsg( const QString &msg );
    void addContact( const QString &jid, const QString& msg = QString() );
    void showAddFriendDialog();

protected:
    virtual QString defaultSuffix() const;

private slots:
    void showXmlConsole();
    void onConnect();
    void onDisconnect(Jreen::Client::DisconnectReason reason);

    void onPresenceReceived( const Jreen::RosterItem::Ptr &item, const Jreen::Presence& presence );
    void onSubscriptionReceived( const Jreen::RosterItem::Ptr &item, const Jreen::Presence& presence );
    void onSubscriptionRequestConfirmed( int result );

    void onNewMessage( const Jreen::Message& message );
    void onError( const Jreen::Connection::SocketError& e );
    void onNewIq( const Jreen::IQ &iq );
    void onNewAvatar( const QString &jid );

private:
    bool readXmlConsoleEnabled();
    QString readUsername();
    QString readPassword();
    QString readServer();
    int readPort();

    QString errorMessage( Jreen::Client::DisconnectReason reason );
    void setupClientHelper();
    void addMenuHelper();
    void removeMenuHelper();

    bool presenceMeansOnline( Jreen::Presence::Type p );
    void handlePeerStatus( const Jreen::JID &jid, Jreen::Presence::Type presenceType );

    using SipPlugin::errorMessage;

    QMenu* m_menu;
    XmlConsole* m_xmlConsole;
    QString m_currentUsername;
    QString m_currentPassword;
    QString m_currentServer;
    int m_currentPort;
    ConnectionState m_state;

    QString m_currentResource;

    // sort out
    Jreen::Client *m_client;

    Jreen::MUCRoom *m_room;
    Jreen::SimpleRoster *m_roster;
    QHash<Jreen::JID, Jreen::Presence::Type> m_peers;
    QHash<Jreen::JID, QMessageBox*> m_subscriptionConfirmBoxes;
    enum IqContext { NoContext, RequestDisco, RequestedDisco, SipMessageSent, RequestedVCard, RequestVersion, RequestedVersion };
    AvatarManager *m_avatarManager;
};

#endif
