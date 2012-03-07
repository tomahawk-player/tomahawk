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

#ifndef LASTFMCONFIG_H
#define LASTFMCONFIG_H

#include <QWidget>

class Ui_LastFmConfig;

namespace Tomahawk {
namespace Accounts {

class LastFmAccount;

class LastFmConfig : public QWidget
{
    Q_OBJECT
public:
    explicit LastFmConfig( LastFmAccount* account );

    QString username() const;
    QString password() const;
    bool scrobble() const;

public slots:
    void testLogin( bool );
    void onLastFmFinished();

private:
    LastFmAccount* m_account;
    Ui_LastFmConfig* m_ui;
};

}
}

#endif // LASTFMCONFIG_H
