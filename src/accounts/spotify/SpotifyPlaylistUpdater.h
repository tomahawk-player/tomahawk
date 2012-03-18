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

#ifndef SPOTIFYPLAYLISTUPDATER_H
#define SPOTIFYPLAYLISTUPDATER_H

#include "playlist/PlaylistUpdaterInterface.h"

namespace Tomahawk {
namespace Accounts {
    class SpotifyAccount;
}
}


class SpotifyPlaylistUpdater : public Tomahawk::PlaylistUpdaterInterface
{
    Q_OBJECT
public:
    SpotifyPlaylistUpdater( Tomahawk::Accounts::SpotifyAccount* acct, const Tomahawk::playlist_ptr& pl );

    virtual ~SpotifyPlaylistUpdater();

    virtual QString type() const;
    virtual void updateNow();

protected:
    virtual void removeFromSettings(const QString& group) const;
    virtual void saveToSettings(const QString& group) const;
    virtual void loadFromSettings(const QString& group);

private:
    Tomahawk::Accounts::SpotifyAccount* m_spotify;
};


class SpotifyUpdaterFactory : public Tomahawk::PlaylistUpdaterFactory
{
public:
    SpotifyUpdaterFactory() : m_account( 0 ) {}

    virtual Tomahawk::PlaylistUpdaterInterface* create( const Tomahawk::playlist_ptr& pl );
    virtual QString type() const { return "spotify"; }

private:
    Tomahawk::Accounts::SpotifyAccount* m_account;
};

#endif // SPOTIFYPLAYLISTUPDATER_H
