/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Leo Franchi <lfranchi@kde.org>
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

#ifndef LASTFMACCOUNT_H
#define LASTFMACCOUNT_H

#include "accounts/Account.h"
#include "AtticaManager.h"
#include "DllMacro.h"

#include <attica/content.h>

#include <QObject>
#include <QSet>

namespace Tomahawk
{
    class ExternalResolverGui;

namespace InfoSystem
{
    class LastFmInfoPlugin;
}

namespace Accounts
{

class LastFmConfig;

class DLLEXPORT LastFmAccountFactory : public AccountFactory
{
    Q_OBJECT
public:
    LastFmAccountFactory();

    virtual Account* createAccount(const QString& accountId = QString());
    virtual QString description() const { return tr( "Scrobble your tracks to last.fm, and find freely downloadable tracks to play" ); }
    virtual QString factoryId() const { return "lastfmaccount"; }
    virtual QString prettyName() const { return "Last.fm"; }
    virtual AccountTypes types() const { return AccountTypes( InfoType | StatusPushType ); }
    virtual bool allowUserCreation() const { return false; }
    virtual QPixmap icon() const;
    virtual bool isUnique() const { return true; }
};

/**
 * Last.Fm account is special. It is both an attica resolver *and* a InfoPlugin. We always want the infoplugin,
 * but the user can install the attica resolver on-demand. So we take care of both there.
 *
 */
class DLLEXPORT LastFmAccount : public CustomAtticaAccount
{
    Q_OBJECT
public:
    explicit LastFmAccount( const QString& accountId );
    ~LastFmAccount();

    virtual void deauthenticate();
    virtual void authenticate();

    virtual SipPlugin* sipPlugin() { return 0; }
    virtual Tomahawk::InfoSystem::InfoPluginPtr infoPlugin();

    virtual bool isAuthenticated() const;

    virtual ConnectionState connectionState() const;
    virtual QPixmap icon() const;
    virtual QWidget* aclWidget() { return 0; }
    virtual QWidget* configurationWidget();
    virtual void saveConfig();

    QString username() const;
    void setUsername( const QString& );
    QString password() const;
    void setPassword( const QString& );
    QString sessionKey() const;
    void setSessionKey( const QString& );
    bool scrobble() const;
    void setScrobble( bool scrobble );

    Attica::Content atticaContent() const;

private slots:
    void atticaLoaded( Attica::Content::List );

    void resolverInstalled( const QString& resolverId );

    void resolverChanged();
private:
    void hookupResolver();

    QWeakPointer<Tomahawk::ExternalResolverGui> m_resolver;
    QWeakPointer<Tomahawk::InfoSystem::LastFmInfoPlugin> m_infoPlugin;
    QWeakPointer<LastFmConfig> m_configWidget;
};

}

}

#endif // LASTFMACCOUNT_H
