/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
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

#ifndef TWITTERACCOUNT_H
#define TWITTERACCOUNT_H

#include "TwitterConfigWidget.h"
#include "TomahawkOAuthTwitter.h"

//#include "sip/TwitterSip.h"
#include "TwitterInfoPlugin.h"
#include "accounts/AccountDllMacro.h"
#include "accounts/Account.h"

#define MYNAME "ACCOUNTTWITTER"

namespace Tomahawk
{

namespace Accounts
{

class ACCOUNTDLLEXPORT TwitterAccountFactory : public AccountFactory
{
    Q_OBJECT
    Q_INTERFACES( Tomahawk::Accounts::AccountFactory )

public:
    TwitterAccountFactory() {}
    virtual ~TwitterAccountFactory() {}

    QString prettyName() const { return "Twitter"; }
    QString factoryId() const { return "twitteraccount"; }
    QString description() const { return tr( "Send tweets from Tomahawk." ); }
    QPixmap icon() const { return QPixmap( ":/twitter-account/twitter-icon.png" ); }
    AccountTypes types() const { return AccountTypes( StatusPushType ); };
    Account* createAccount( const QString& pluginId = QString() );
};

class ACCOUNTDLLEXPORT TwitterAccount : public Account
{
    Q_OBJECT

public:
    TwitterAccount( const QString &accountId );
    virtual ~TwitterAccount();

    QPixmap icon() const;

    void authenticate();
    void deauthenticate();
    bool isAuthenticated() const { return m_isAuthenticated; }

    ConnectionState connectionState() const;

    Tomahawk::InfoSystem::InfoPluginPtr infoPlugin();
    SipPlugin* sipPlugin();

    AccountConfigWidget* configurationWidget() { return m_configWidget.data(); }
    QWidget* aclWidget() { return 0; }

    bool refreshTwitterAuth();
    TomahawkOAuthTwitter* twitterAuth() const { return m_twitterAuth.data(); }

signals:
    void nowAuthenticated( const QPointer< TomahawkOAuthTwitter >&, const QTweetUser &user );
    void nowDeauthenticated();

private slots:
    void authenticateSlot();
    void configDialogAuthedSignalSlot( bool authed );
    void connectAuthVerifyReply( const QTweetUser &user );

private:
    QIcon m_icon;
    bool m_isAuthenticated;
    bool m_isAuthenticating;
    QPointer< TomahawkOAuthTwitter > m_twitterAuth;
    QPointer< TwitterConfigWidget > m_configWidget;
//    QPointer< TwitterSipPlugin > m_twitterSipPlugin;
    QPointer< Tomahawk::InfoSystem::TwitterInfoPlugin > m_twitterInfoPlugin;

    // for settings access
    friend class TwitterConfigWidget;

    QPixmap m_onlinePixmap;
    QPixmap m_offlinePixmap;
};

};

};

#endif
