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
    XspfUpdater( const playlist_ptr& pl, const QString& xspfUrl );
    XspfUpdater( const playlist_ptr& pl, int interval, bool autoUpdate, const QString& xspfUrl );
    explicit XspfUpdater( const playlist_ptr& pl ); // used by factory

    virtual ~XspfUpdater();

    virtual QString type() const { return "xspf"; }
public slots:
    void updateNow();

protected:
    void loadFromSettings( const QString& group );
    void saveToSettings( const QString& group ) const;
    virtual void removeFromSettings(const QString& group) const;

private slots:
    void playlistLoaded();

private:
    QString m_url;
};

}

#endif // XSPFUPDATER_H
