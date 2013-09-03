/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef HATCHET_SIP_H
#define HATCHET_SIP_H

#include "accounts/AccountDllMacro.h"
#include "database/Op.h"
#include "sip/SipPlugin.h"
#include "account/HatchetAccount.h"

#include <QPointer>
#include <QTimer>
#include <QtCrypto>

class WebSocketThreadController;

const int VERSION = 1;

class ACCOUNTDLLEXPORT HatchetSipPlugin : public SipPlugin
{
    Q_OBJECT

    enum SipState {
        AcquiringVersion,
        Registering,
        Connected,
        Closed
    };

public:
    HatchetSipPlugin( Tomahawk::Accounts::Account *account );

    virtual ~HatchetSipPlugin();

    virtual bool isValid() const;

    virtual void sendSipInfos( const Tomahawk::peerinfo_ptr& receiver, const QList< SipInfo >& infos );

public slots:
    virtual void connectPlugin();
    void disconnectPlugin();
    void checkSettings() {}
    void configurationChanged() {}
    bool addContact( const QString&, AddContactOptions, const QString& ) { return false; }
    void sendMsg( const QString&, const SipInfo& ) {}
    void webSocketConnected();
    void webSocketDisconnected();

signals:
    void connectWebSocket() const;
    void disconnectWebSocket() const;
    void authUrlDiscovered( Tomahawk::Accounts::HatchetAccount::Service service, const QString& authUrl ) const;
    void rawBytes( QByteArray bytes ) const;

private slots:
    void dbSyncTriggered();
    void messageReceived( const QByteArray& msg );
    void connectWebSocket();
    void oplogFetched( const QString& sinceguid, const QString& lastguid, const QList< dbop_ptr > ops );

private:
    bool sendBytes( const QVariantMap& jsonMap ) const;
    bool checkKeys( QStringList keys, const QVariantMap& map ) const;
    void newPeer( const QVariantMap& valMap );
    void peerAuthorization( const QVariantMap& valMap );
    void sendOplog( const QVariantMap& valMap ) const;
    Tomahawk::Accounts::HatchetAccount* hatchetAccount() const;

    QPointer< WebSocketThreadController > m_webSocketThreadController;
    QString m_token;
    QString m_uuid;
    SipState m_sipState;
    int m_version;
    QCA::PublicKey* m_publicKey;
    QTimer m_reconnectTimer;
    QHash< QString, QList< SipInfo > > m_sipInfoHash;
};

#endif
