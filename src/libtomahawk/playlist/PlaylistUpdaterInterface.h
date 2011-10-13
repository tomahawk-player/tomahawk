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
#include <QTimer>

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
    PlaylistUpdaterInterface( const playlist_ptr& pl );

    virtual ~PlaylistUpdaterInterface(){}

    // What type you are. If you add a new updater, add the creation code as well.
    virtual QString type() const = 0;

    bool autoUpdate() const { return m_autoUpdate; }
    void setAutoUpdate( bool autoUpdate );

    void setInterval( int intervalMsecs ) ;
    int intervalMsecs() const { return m_timer->interval(); }

    void remove();

    playlist_ptr playlist() const { return m_playlist; }

    /// If you want to try to load a updater from the settings. Returns a valid
    /// updater if one was saved
    static PlaylistUpdaterInterface* loadForPlaylist( const playlist_ptr& pl );

public slots:
    virtual void updateNow() {}

private slots:
    void doSave();

protected:
    virtual void loadFromSettings( const QString& group ) = 0;
    virtual void saveToSettings( const QString& group ) const = 0;
    virtual void removeFromSettings( const QString& group ) const = 0;

private:
    QTimer* m_timer;
    bool m_autoUpdate;
    playlist_ptr m_playlist;
};

}

#endif // PLAYLISTUPDATERINTERFACE_H
