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

#ifndef LASTFMCONFIG_H
#define LASTFMCONFIG_H

#include "Query.h"
#include "accounts/AccountConfigWidget.h"
#include "database/DatabaseCommand_LoadSocialActions.h"

#include <QWidget>
#include <QSet>
#include <QNetworkReply>

class Ui_LastFmConfig;

namespace Tomahawk {

namespace Accounts {

class LastFmAccount;

class LastFmConfig : public AccountConfigWidget
{
    Q_OBJECT
public:
    explicit LastFmConfig( LastFmAccount* account );

    QString username() const;
    QString password() const;
    bool scrobble() const;

public slots:
    void testLogin();
    void onLastFmFinished();

private slots:
    void enableButton();

    void loadHistory();
    void onHistoryLoaded();

    void syncLovedTracks() { syncLovedTracks( 1 ); }
    void syncLovedTracks( uint page );
    void onLovedFinished( QNetworkReply* reply );
    void localLovedLoaded( DatabaseCommand_LoadSocialActions::TrackActions );

signals:
    void sizeHintChanged();

private:
    void syncLoved();

    LastFmAccount* m_account;
    Ui_LastFmConfig* m_ui;

    unsigned int m_page;
    unsigned int m_lastTimeStamp;

    int m_totalLovedPages;
    bool m_doneFetchingLoved, m_doneFetchingLocal;
    QSet< Tomahawk::query_ptr > m_lastfmLoved;
    DatabaseCommand_LoadSocialActions::TrackActions m_localLoved;
};

}

}

#endif // LASTFMCONFIG_H
