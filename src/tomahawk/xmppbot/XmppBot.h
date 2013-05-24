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

#ifndef XMPPBOT_H
#define XMPPBOT_H

#include <result.h>
#include <infosystem/InfoSystem.h>

#include <QtCore/QObject>
#include <QtCore/qsharedpointer.h>
#include <QTimer>

#include <gloox/messagehandler.h>
#include <gloox/messagesessionhandler.h>
#include <gloox/client.h>
#include <gloox/connectionlistener.h>
#include <gloox/subscriptionhandler.h>
#include <gloox/messagehandler.h>

class XMPPBotClient
    : public QObject
    , public gloox::Client
{
    Q_OBJECT

public:
    XMPPBotClient(QObject* parent, gloox::JID &jid, std::string password, int port);
    virtual ~XMPPBotClient();

    void run();

private slots:
    void recvSlot();

private:
    QTimer m_timer;
};

class XMPPBot
    : public QObject
    , public gloox::ConnectionListener
    , public gloox::SubscriptionHandler
    , public gloox::MessageHandler
{
    Q_OBJECT

public:
    XMPPBot(QObject *parent);
    virtual ~XMPPBot();

public slots:
    virtual void newTrackSlot(const Tomahawk::result_ptr &track);
    virtual void infoReturnedSlot(QString caller, Tomahawk::InfoSystem::InfoType type, QVariant input, QVariant output, QVariantMap customData);
    virtual void infoFinishedSlot(QString caller);

protected:
    // ConnectionListener
    virtual void onConnect();
    virtual void onDisconnect(gloox::ConnectionError e);
    virtual bool onTLSConnect(const gloox::CertInfo &info);

    // SubscriptionHandler
    virtual void handleSubscription(const gloox::Subscription &subscription);

    // MessageHandler
    virtual void handleMessage(const gloox::Message &msg, gloox::MessageSession *session = 0);

private slots:
    void onResultsAdded( const QList<Tomahawk::result_ptr>& result );

private:
    QPointer<XMPPBotClient> m_client;
    Tomahawk::result_ptr m_currTrack;
    Tomahawk::InfoSystem::InfoTypeMap m_currInfoMap;
    QString m_currReturnMessage;
    QString m_currReturnJid;
};

#endif // XMPPBOT_H
