/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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
#include "ResolverAccount.h"
class QTimer;

namespace Tomahawk {

class ExternalResolverGui;

namespace Accounts {

class SpotifyResolverAccount : public QObject // : public AccountFactory
{
    Q_OBJECT
public:
    SpotifyResolverAccount();
    virtual ~SpotifyResolverAccount();

    /*virtual Account* createAccount(const QString& accountId = QString());
    virtual QString description() const { return tr( "Play and sync your playlists with Spotify" ); }
    virtual QString factoryId() const { return "spotifyaccount"; }
    virtual QString prettyName() const { return "Spotify"; }

    virtual AccountTypes types() const { return AccountTypes( ResolverType ); }
    virtual bool allowUserCreation() const { return false; }
    virtual QPixmap icon() const { return m_icon; }
    virtual bool isUnique() const { return true; }
    */
    void addPlaylist(const QString &qid, const QString& title, QList< Tomahawk::query_ptr > tracks);
    struct Sync {
         QString id_;
         QString uuid;
         Tomahawk::playlist_ptr playlist;
     };

private:
    QList<Sync> m_syncPlaylists;
    int m_sCount;

};
}

}

#endif // SpotifyAccount_H
