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
#include <QMutex>

#ifndef ENABLE_HEADLESS
#include <QPixmap>
#endif

namespace Tomahawk
{
/**
  * PlaylistUpdaters are attached to playlists. They usually manipulate the playlist in some way
  * due to external input (spotify syncing) or timers (xspf updating)
  */

class PlaylistUpdaterFactory;

class DLLEXPORT PlaylistUpdaterInterface : public QObject
{
    Q_OBJECT
public:
    explicit PlaylistUpdaterInterface( const playlist_ptr& pl );

    virtual ~PlaylistUpdaterInterface(){}

    // What type you are. If you add a new updater, add the creation code as well.
    virtual QString type() const = 0;

#ifndef ENABLE_HEADLESS
    // Small widget to show in playlist header that configures the updater
    virtual QWidget* configurationWidget() const = 0;

    // Small overlay over playlist icon in the sidebar to indicate that it has this updater type
    // Should be around 16x16 or something
    virtual QPixmap typeIcon() const { return QPixmap(); }
#endif

    void remove();

    playlist_ptr playlist() const { return m_playlist; }

    /// If you want to try to load a updater from the settings. Returns a valid
    /// updater if one was saved
    static PlaylistUpdaterInterface* loadForPlaylist( const playlist_ptr& pl );

    static void registerUpdaterFactory( PlaylistUpdaterFactory* f );

signals:
    void changed();

public slots:
    virtual void updateNow() {}

    void save();

protected:
    virtual void saveToSettings( const QString& group ) const = 0;
    virtual void removeFromSettings( const QString& group ) const = 0;

private:
    playlist_ptr m_playlist;

    static QMap< QString, PlaylistUpdaterFactory* > s_factories;
};


class DLLEXPORT PlaylistUpdaterFactory
{
public:
    PlaylistUpdaterFactory() {}
    virtual ~PlaylistUpdaterFactory() {}

    virtual QString type() const = 0;
    virtual PlaylistUpdaterInterface* create( const playlist_ptr&, const QString& settingsKey ) = 0;
};

}

#endif // PLAYLISTUPDATERINTERFACE_H
