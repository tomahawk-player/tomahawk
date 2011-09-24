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

#ifndef PLAYLISTUPDATERINTERFACE_H
#define PLAYLISTUPDATERINTERFACE_H

#include "dllmacro.h"
#include "typedefs.h"
#include "playlist.h"

namespace Tomahawk
{
/**
  * If a playlist needs periodic updating, implement a updater interface.
  *
  * Default is auto-updating.
  */

class DLLEXPORT PlaylistUpdaterInterface : public QObject
{
    Q_OBJECT
public:
    PlaylistUpdaterInterface( const playlist_ptr& pl, QObject* parent )
        : QObject( parent )
        , m_autoUpdate( true )
        , m_playlist( pl )
    {}

    virtual ~PlaylistUpdaterInterface() {}

    void setAutoUpdate( bool autoUpdate ) { m_autoUpdate = autoUpdate; }
    bool autoUpdate() const { return m_autoUpdate; }

    playlist_ptr playlist() const { return m_playlist; }
signals:

private:
    bool m_autoUpdate;
    playlist_ptr m_playlist;
};

}

#endif // PLAYLISTUPDATERINTERFACE_H
