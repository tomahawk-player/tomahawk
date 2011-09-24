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

#ifndef XSPFUPDATER_H
#define XSPFUPDATER_H

#include "PlaylistUpdaterInterface.h"

class QTimer;

namespace Tomahawk
{

class XspfUpdater : public PlaylistUpdaterInterface
{
    Q_OBJECT
public:
    explicit XspfUpdater( const playlist_ptr& pl, const QString& xspfUrl, QObject *parent = 0 );
    virtual ~XspfUpdater();

private slots:
    void update();
    void playlistLoaded();

private:
    QString m_url;
    QTimer* m_timer;
};

}

#endif // XSPFUPDATER_H
