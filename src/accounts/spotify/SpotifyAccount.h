/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Hugo Lindström <hugolm84@gmail.com>
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

#ifndef SpotifyAccount_H
#define SpotifyAccount_H

#include "playlist.h"
#include "utils/tomahawkutils.h"
#include "sourcelist.h"
#include "accounts/ResolverAccount.h"

class QTimer;

class ScriptResolver;

namespace Tomahawk {
namespace Accounts {

class SpotifyAccountConfig;

class SpotifyAccountFactory : public AccountFactory
{
    Q_OBJECT
public:
    SpotifyAccountFactory() {}

    virtual Account* createAccount( const QString& accountId = QString() );
    virtual QString description() const { return tr( "Play music from and sync your playlists with Spotify" ); }
    virtual QString factoryId() const { return "spotifyaccount"; }
    virtual QString prettyName() const { return "Spotify"; }

    virtual bool acceptsPath( const QString& path ) const;
    virtual Account* createFromPath( const QString& path );

    virtual AccountTypes types() const { return AccountTypes( ResolverType ); }
    virtual bool allowUserCreation() const { return false; }
    virtual QPixmap icon() const;
    virtual bool isUnique() const { return true; }

};

class SpotifyAccount : public ResolverAccount
{
    Q_OBJECT
public:
    SpotifyAccount( const QString& accountId );
    SpotifyAccount( const QString& accountId, const QString& path );
    virtual ~SpotifyAccount() {}

    virtual QPixmap icon() const;
    virtual QWidget* configurationWidget();
    virtual void saveConfig();

    virtual QWidget* aclWidget() { return 0; }
    virtual InfoSystem::InfoPlugin* infoPlugin() { return 0; }
    virtual SipPlugin* sipPlugin() { return 0; }

    void addPlaylist( const QString &qid, const QString& title, QList< Tomahawk::query_ptr > tracks );

    struct Sync {
         QString id_;
         QString uuid;
         Tomahawk::playlist_ptr playlist;
     };

private slots:
    void resolverMessage( const QString& msgType, const QVariantMap& msg );

private:
    void init();

    QList<Sync> m_syncPlaylists;
    QWeakPointer<SpotifyAccountConfig> m_configWidget;
    QWeakPointer<ScriptResolver> m_spotifyResolver;
};

}
}

#endif // SpotifyAccount_H
