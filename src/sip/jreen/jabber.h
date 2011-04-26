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
#include "jabber_p.h"

#include "../sipdllmacro.h"

#define MYNAME "SIPJREEN"

class SIPDLLEXPORT JabberFactory : public SipPluginFactory
{
    Q_OBJECT
    Q_INTERFACES( SipPluginFactory )

public:
    JabberFactory() {}
    virtual ~JabberFactory() {}

    virtual QString prettyName() { return "Jabber"; }
    virtual QString factoryId() { return "sipjabber"; }
    virtual SipPlugin* createPlugin( const QString& pluginId );
};

class SIPDLLEXPORT JabberPlugin : public SipPlugin
{
    Q_OBJECT

public:
    JabberPlugin( const QString& pluginId );
    virtual ~JabberPlugin();

    //FIXME: Make this more correct
    virtual bool isValid() const { return true; }
    virtual const QString name() const;
    virtual const QString friendlyName() const;
    virtual const QString accountName() const;
    virtual ConnectionState connectionState() const;
    virtual QMenu* menu();

    void setProxy( QNetworkProxy* proxy );

public slots:
    virtual bool connectPlugin( bool startup );
    void disconnectPlugin();
    void checkSettings();
    void sendMsg( const QString& to, const QString& msg );
    void broadcastMsg( const QString &msg );
    void addContact( const QString &jid, const QString& msg = QString() );

private slots:
    void showAddFriendDialog();
    void onConnected();
    void onDisconnected();
    void onAuthError(int code, const QString &msg);

private:
    QString readPassword();
    QString readServer();
    bool readAutoConnect();
    int readPort();

    Jabber_p* p;
    QMenu* m_menu;
    QAction* m_addFriendAction;

    QString m_currentUsername;
    QString m_currentPassword;
    QString m_currentServer;
    unsigned int m_currentPort;
    ConnectionState m_state;
};

#endif
