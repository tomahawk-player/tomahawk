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
    Jabber_p* p;
    QMenu* m_menu;
    QAction* m_addFriendAction;

    QString m_currentUsername;
    QString m_currentPassword;
    QString m_currentServer;
    unsigned int m_currentPort;
};

#endif
