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

#ifndef XMPPACCOUNT_H
#define XMPPACCOUNT_H

#include "sip/XmppSip.h"
#include "accounts/AccountDllMacro.h"
#include "accounts/Account.h"

#define MYNAME "ACCOUNTJABBER"

class Ui_XmppConfigWidget;

namespace Tomahawk
{

namespace Accounts
{

class ACCOUNTDLLEXPORT XmppAccountFactory : public AccountFactory
{
    Q_OBJECT
    Q_INTERFACES( Tomahawk::Accounts::AccountFactory )

    // for settings access
    friend class XmppConfigWidget;
public:
    XmppAccountFactory() {}
    virtual ~XmppAccountFactory() {}

    QString prettyName() const { return "Jabber (XMPP)"; }
    QString description() const { return tr( "Log on to your Jabber/XMPP account to connect to your friends" ); }
    QString factoryId() const { return "xmppaccount"; }
    QPixmap icon() const { return QPixmap( ":/xmpp-account/xmpp-icon.png" ); }
    AccountTypes types() const { return AccountTypes( SipType | StatusPushType ); };
    Account* createAccount( const QString& pluginId = QString() );
};

class ACCOUNTDLLEXPORT XmppAccount : public Account
{
    Q_OBJECT

public:
    XmppAccount( const QString &accountId );
    virtual ~XmppAccount();

    QPixmap icon() const;

    void authenticate();
    void deauthenticate();
    bool isAuthenticated() const;

    Tomahawk::InfoSystem::InfoPluginPtr infoPlugin();

    SipPlugin* sipPlugin();

    AccountConfigWidget* configurationWidget() { return m_configWidget.data(); }
    QWidget* aclWidget() { return 0; }
    void saveConfig();

    virtual Tomahawk::Accounts::Account::ConnectionState connectionState() const;

protected:
    QPointer< AccountConfigWidget > m_configWidget; // so the google wrapper can change the config dialog a bit
    QPointer< XmppSipPlugin > m_xmppSipPlugin;
    QPointer< Tomahawk::InfoSystem::XmppInfoPlugin > m_xmppInfoPlugin;

    QPixmap m_onlinePixmap;
    QPixmap m_offlinePixmap;
};

};

};

#endif
