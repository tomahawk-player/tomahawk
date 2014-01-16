/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
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

#ifndef HATCHET_ACCOUNT_H
#define HATCHET_ACCOUNT_H

#include "TomahawkPlugin.h"
#include <accounts/Account.h>
#include <accounts/AccountDllMacro.h>

#include <QtCrypto>

class QNetworkReply;

class HatchetSipPlugin;

namespace Tomahawk
{
namespace Accounts
{

class HatchetAccountConfig;

class ACCOUNTDLLEXPORT HatchetAccountFactory : public AccountFactory
{
    Q_OBJECT
    Q_INTERFACES( Tomahawk::Accounts::AccountFactory )
    Q_PLUGIN_METADATA( IID "org.tomahawk-player.Player.AccountFactory" )

public:
    HatchetAccountFactory();
    virtual ~HatchetAccountFactory();

    virtual QString factoryId() const { return "hatchetaccount"; }
    virtual QString prettyName() const { return "Hatchet"; }
    virtual QString description() const { return tr( "Connect to your Hatchet account" ); }
    virtual bool isUnique() const { return true; }
    AccountTypes types() const { return AccountTypes( SipType ); }
//    virtual bool allowUserCreation() const { return false; }
    virtual QPixmap icon() const;

    virtual Account* createAccount ( const QString& pluginId = QString() );
};

class ACCOUNTDLLEXPORT HatchetAccount : public Account
{
    Q_OBJECT
public:
    enum Service {
        Facebook = 0
    };

    HatchetAccount( const QString &accountId );
    virtual ~HatchetAccount();

    static HatchetAccount* instance();

    QPixmap icon() const;

    void authenticate();
    void deauthenticate();
    bool isAuthenticated() const;

    void setConnectionState( Account::ConnectionState connectionState );
    ConnectionState connectionState() const;

    virtual Tomahawk::InfoSystem::InfoPluginPtr infoPlugin() { return Tomahawk::InfoSystem::InfoPluginPtr(); }
    SipPlugin* sipPlugin( bool create = true );

    AccountConfigWidget* configurationWidget();
    QWidget* aclWidget() { return 0; }

    QString username() const;

    void fetchAccessToken( const QString& type = "dreamcatcher" );

    QString authUrlForService( const Service& service ) const;

signals:
    void authError( QString error, int statusCode, const QVariantMap );
    void deauthenticated();
    void accessTokenFetched();

private slots:
    void onPasswordLoginFinished( QNetworkReply*, const QString& username );
    void onFetchAccessTokenFinished( QNetworkReply*, const QString& type );
    void authUrlDiscovered( Tomahawk::Accounts::HatchetAccount::Service service, const QString& authUrl );

private:
    QByteArray refreshToken() const;
    uint refreshTokenExpiration() const;

    QByteArray mandellaAccessToken() const;
    uint mandellaAccessTokenExpiration() const;

    QByteArray mandellaTokenType() const;

    void loginWithPassword( const QString& username, const QString& password, const QString &otp );

    QVariantMap parseReply( QNetworkReply* reply, bool& ok ) const;

    QPointer<HatchetAccountConfig> m_configWidget;

    Account::ConnectionState m_state;

    QPointer< HatchetSipPlugin > m_tomahawkSipPlugin;
    QHash< Service, QString > m_extraAuthUrls;

    static HatchetAccount* s_instance;
    friend class HatchetAccountConfig;

    QCA::PublicKey* m_publicKey;
    QString m_uuid;
};

}
}


#endif
