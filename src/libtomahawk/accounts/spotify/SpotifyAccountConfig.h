/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012, Leo Franchi <lfranchi@kde.org>
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

#ifndef SPOTIFYACCOUNTCONFIG_H
#define SPOTIFYACCOUNTCONFIG_H

#include "accounts/AccountConfigWidget.h"

#include <QWidget>
#include <QVariantMap>
#include <QTimer>

class QLabel;
class AnimatedSpinner;
class QShowEvent;

namespace Ui
{
    class SpotifyConfig;
}

namespace Tomahawk
{
namespace Accounts
{

class SpotifyAccount;
struct SpotifyPlaylistInfo;

class SpotifyAccountConfig : public AccountConfigWidget
{
    Q_OBJECT
public:
    explicit SpotifyAccountConfig( SpotifyAccount* account );

    QString username() const;
    QString password() const;
    bool highQuality() const;
    bool deleteOnUnsync() const;
    bool loveSync() const;

    void setPlaylists( const QList< SpotifyPlaylistInfo* >& playlists );

    void loadFromConfig();
    void saveSettings();

    void loginResponse( bool success, const QString& msg, const QString& username );

    bool loggedInManually() const { return m_loggedInManually; }

signals:
    void login( const QString& username, const QString& pw );
    void logout();

protected:
    void showEvent( QShowEvent* event );

private slots:
    void doLogin();
    void resetLoginButton();
    void selectAllPlaylists();
    void showStarredPlaylist(bool);

private:
    void showLoggedIn();
    void showLoggedOut();

    Ui::SpotifyConfig* m_ui;
    QLabel* m_loggedInUser;
    QString m_verifiedUsername;
    SpotifyAccount* m_account;
    AnimatedSpinner* m_playlistsLoading;
    bool m_loggedInManually, m_isLoggedIn;
};

}
}

#endif // SPOTIFYACCOUNTCONFIG_H
