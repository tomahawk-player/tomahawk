/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#ifndef ZEROCONF_ACCOUNTS_H
#define ZEROCONF_ACCOUNTS_H

#include "TomahawkPlugin.h"
#include "Zeroconf.h"
#include "accounts/Account.h"
#include "accounts/AccountDllMacro.h"

class SipPlugin;

namespace Tomahawk
{
namespace Accounts
{

class ACCOUNTDLLEXPORT ZeroconfFactory : public AccountFactory
{
    Q_PLUGIN_METADATA( IID "org.tomahawk-player.Player.AccountFactory" )
    Q_OBJECT
    Q_INTERFACES( Tomahawk::Accounts::AccountFactory )

public:
    ZeroconfFactory();
    virtual ~ZeroconfFactory();

    QString factoryId() const override { return "zeroconfaccount"; }
    QString prettyName() const override { return tr( "Local Network" ); }
    QString description() const override { return tr( "Automatically connect to Tomahawk users on the same local network." ); }
    bool isUnique() const override { return true; }
    AccountTypes types() const override { return AccountTypes( SipType ); }
    QPixmap icon() const override;

    Account* createAccount ( const QString& pluginId = QString() ) override;
};

class ACCOUNTDLLEXPORT ZeroconfAccount : public Account
{
    Q_OBJECT
public:
    ZeroconfAccount( const QString &accountId );
    virtual ~ZeroconfAccount();

    QPixmap icon() const override;

    void authenticate() override;
    void deauthenticate() override;
    bool isAuthenticated() const override;
    ConnectionState connectionState() const override;

    Tomahawk::InfoSystem::InfoPluginPtr infoPlugin() override { return Tomahawk::InfoSystem::InfoPluginPtr(); }
    SipPlugin* sipPlugin( bool create = true ) override;

    AccountConfigWidget* configurationWidget() override { return nullptr; }
    QWidget* aclWidget() override { return nullptr; }

private:
    QPointer< ZeroconfPlugin > m_sipPlugin;
};

}
}

#endif
