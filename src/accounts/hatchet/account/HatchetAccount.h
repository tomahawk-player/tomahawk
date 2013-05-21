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

#ifndef TOMAHAWK_ACCOUNT_H
#define TOMAHAWK_ACCOUNT_H


#include <accounts/Account.h>
#include <accounts/AccountDllMacro.h>

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
public:
    HatchetAccountFactory();
    virtual ~HatchetAccountFactory();

    virtual QString factoryId() const { return "hatchetaccount"; }
    virtual QString prettyName() const { return "Hatchet"; }
    virtual QString description() const { return tr( "Connect to your Hatchet account" ); }
    virtual bool isUnique() const { return true; }
    AccountTypes types() const { return AccountTypes( SipType ); };
//    virtual bool allowUserCreation() const { return false; }
#ifndef ENABLE_HEADLESS
    virtual QPixmap icon() const;
#endif


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
    SipPlugin* sipPlugin();

    AccountConfigWidget* configurationWidget();
    QWidget* aclWidget() { return 0; }

    QString username() const;

    void fetchAccessTokens();

    QString authUrlForService( const Service& service ) const;

signals:
    void deauthenticated();
    void accessTokensFetched();
    void registerFinished( bool successful, const QString& msg );

private slots:
    void onRegisterFinished();
    void onPasswordLoginFinished( QNetworkReply*, const QString& username );
    void onFetchAccessTokensFinished();

    void authUrlDiscovered( Tomahawk::Accounts::HatchetAccount::Service service, const QString& authUrl );
private:
    QByteArray authToken() const;

    void doRegister( const QString& username, const QString& password, const QString& email );
    void loginWithPassword( const QString& username, const QString& password );

    QNetworkReply* buildRequest( const QString& command, const QVariantMap& params ) const;
    QVariantMap parseReply( QNetworkReply* reply, bool& ok ) const;

    QWeakPointer<HatchetAccountConfig> m_configWidget;

    Account::ConnectionState m_state;

    QWeakPointer< HatchetSipPlugin > m_tomahawkSipPlugin;
    QHash< Service, QString > m_extraAuthUrls;

    static HatchetAccount* s_instance;
    friend class HatchetAccountConfig;
};

}
}


#endif
