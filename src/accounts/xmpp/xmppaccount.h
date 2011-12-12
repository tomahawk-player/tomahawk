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

#ifndef XMPPACCOUNT_H
#define XMPPACCOUNT_H

#include "sip/xmppsip.h"
#include "accounts/accountdllmacro.h"
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

public:
    XmppAccountFactory() {}
    virtual ~XmppAccountFactory() {}

    QString prettyName() const { return "XMPP (Jabber)"; }
    QString factoryId() const { return "xmppaccount"; }
    QIcon icon() const { return QIcon( ":/xmpp-icon.png" ); }
    Account* createAccount( const QString& pluginId = QString() );
};

class DLLEXPORT XmppAccount : public Account
{
    Q_OBJECT

public:
    XmppAccount( const QString &accountId );
    virtual ~XmppAccount();

    QIcon icon() const { return QIcon( ":/xmpp-icon.png" ); }

    void authenticate();
    void deauthenticate();
    bool isAuthenticated() const { return m_isAuthenticated; }

    Tomahawk::InfoSystem::InfoPlugin* infoPlugin() { return 0; }
    SipPlugin* sipPlugin();

    QWidget* configurationWidget() { return m_configWidget.data(); }
    QWidget* aclWidget() { return 0; }

    void refreshProxy() {};

private:
    Ui_XmppConfigWidget* m_ui; // so the google wrapper can change the config dialog a bit
    bool m_isAuthenticated;
    QWeakPointer< QWidget > m_configWidget;
    QWeakPointer< XmppSipPlugin > m_xmppSipPlugin;


    // for settings access
    friend class XmppConfigWidget;
};

};

};

#endif
