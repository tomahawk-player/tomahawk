/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Dominik Schmidt <dev@dominik-schmidt.de>
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef XMPPSIP_H
#define XMPPSIP_H

#include "sip/SipPlugin.h"

#include "AvatarManager.h"

#ifndef ENABLE_HEADLESS
    #include "XmlConsole.h"
#endif

#include "accounts/AccountDllMacro.h"

#include "../XmppInfoPlugin.h"

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
#include <jreen/pubsubmanager.h>

#ifndef ENABLE_HEADLESS
    #include <QMessageBox>
#endif


class ACCOUNTDLLEXPORT XmppSipPlugin : public SipPlugin
{
    Q_OBJECT

friend class Tomahawk::InfoSystem::XmppInfoPlugin;

public:
    XmppSipPlugin( Tomahawk::Accounts::Account* account );
    virtual ~XmppSipPlugin();

    //FIXME: Make this more correct
    virtual bool isValid() const { return true; }
    virtual QString inviteString() const;

    Tomahawk::InfoSystem::InfoPluginPtr infoPlugin();

#ifndef ENABLE_HEADLESS
    virtual QMenu* menu();
#endif

    // used by XmppAccount to expose connection state and controls
    Tomahawk::Accounts::Account::ConnectionState connectionState() const;

signals:
    void jidChanged( const QString& );

    // Used by XmppAccount
    void stateChanged( Tomahawk::Accounts::Account::ConnectionState state );
    void error( int errorId, const QString& errorStr );

public slots:
    virtual void connectPlugin();
    virtual void disconnectPlugin();
    virtual void checkSettings();
    virtual void configurationChanged();
    virtual void addContact( const QString& peerId, const QString& msg = QString() );

    virtual void sendSipInfo( const Tomahawk::peerinfo_ptr& receiver, const SipInfo& info );

    void showAddFriendDialog();
    void publishTune( const QUrl& url, const Tomahawk::InfoSystem::InfoStringHash& trackInfo );

protected:
    virtual QString defaultSuffix() const;

private slots:
    void showXmlConsole();
    void onConnect();
    void onDisconnect( Jreen::Client::DisconnectReason reason );

    void onPresenceReceived( const Jreen::RosterItem::Ptr& item, const Jreen::Presence& presence );
    void onSubscriptionReceived( const Jreen::RosterItem::Ptr& item, const Jreen::Presence& presence );
    void onSubscriptionRequestConfirmed( int result );

    void onNewMessage( const Jreen::Message& message );
    void onError( const Jreen::Connection::SocketError& e );
    void onNewIq( const Jreen::IQ& iq );
    void onNewAvatar( const QString& jid );

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
    void handlePeerStatus( const Jreen::JID& jid, Jreen::Presence::Type presenceType );

    QString m_currentUsername;
    QString m_currentPassword;
    QString m_currentServer;
    int m_currentPort;
    QString m_currentResource;

    QPointer< Tomahawk::InfoSystem::XmppInfoPlugin > m_infoPlugin;
    Tomahawk::Accounts::Account::ConnectionState m_state;

    // sort out
    Jreen::Client* m_client;

    Jreen::MUCRoom* m_room;
    Jreen::SimpleRoster* m_roster;
    QHash< Jreen::JID, Jreen::Presence::Type > m_peers;
    QHash< QString, QString > m_jidsNames;

#ifndef ENABLE_HEADLESS
    QHash< Jreen::JID, QMessageBox* > m_subscriptionConfirmBoxes;
    QMenu* m_menu;
    XmlConsole* m_xmlConsole;
#endif

    enum IqContext { NoContext, RequestDisco, RequestedDisco, SipMessageSent, RequestedVCard, RequestVersion, RequestedVersion };
    AvatarManager* m_avatarManager;
    Jreen::PubSub::Manager* m_pubSubManager;
};

#endif
