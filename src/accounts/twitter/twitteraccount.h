/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
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

#ifndef TWITTERACCOUNT_H
#define TWITTERACCOUNT_H

#include "twitterconfigwidget.h"
#include "tomahawkoauthtwitter.h"

#include "sip/twittersip.h"
#include "accounts/accountdllmacro.h"
#include "accounts/account.h"

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
    QIcon icon() const { return QIcon( ":/twitter-icon.png" ); }
    Account* createAccount( const QString& pluginId = QString() );
};

class DLLEXPORT TwitterAccount : public Account
{
    Q_OBJECT

public:
    TwitterAccount( const QString &accountId );
    virtual ~TwitterAccount();
    
    QIcon icon() const { return QIcon( ":/twitter-icon.png" ); }

    bool canSelfAuthenticate() const { return true; }
    void authenticate();
    void deauthenticate();
    bool isAuthenticated() const { return m_isAuthenticated; }

    Tomahawk::InfoSystem::InfoPlugin* infoPlugin() { return 0; }
    SipPlugin* sipPlugin();

    QWidget* configurationWidget() { return m_configWidget.data(); }
    QWidget* aclWidget() { return 0; }

    bool refreshTwitterAuth();
    TomahawkOAuthTwitter* twitterAuth() const { return m_twitterAuth.data(); }

    void refreshProxy();

signals:
    void nowAuthenticated( const QWeakPointer< TomahawkOAuthTwitter >&, const QTweetUser &user );
    void nowDeauthenticated();
    
private slots:
    void configDialogAuthedSignalSlot( bool authed );
    void connectAuthVerifyReply( const QTweetUser &user );
    
private:
    bool m_isAuthenticated;
    QWeakPointer< TomahawkOAuthTwitter > m_twitterAuth;
    QWeakPointer< TwitterConfigWidget > m_configWidget;
    QWeakPointer< TwitterSipPlugin > m_twitterSipPlugin;

    // for settings access
    friend class TwitterConfigWidget;
};

};

};

#endif
