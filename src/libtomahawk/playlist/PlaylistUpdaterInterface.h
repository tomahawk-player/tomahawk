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

#include "DllMacro.h"
#include "Typedefs.h"
#include "Playlist.h"

#include <QTimer>
#include <QMutex>
#include <QPair>

#ifndef ENABLE_HEADLESS
#include <QPixmap>
#endif

namespace Tomahawk
{
/**
  * PlaylistUpdaters are attached to playlists. They usually manipulate the playlist in some way
  * due to external input (spotify syncing) or timers (xspf updating)
  *
  * Updaters have 2 modes of operation: syncing and subscribing. Syncing implies two-way sync, that is, this
  * playlist is reproduced on some other service (e.g. spotify or rdio).
  *
  * Subscribing implies the playlist is being updated periodically with changes from some source, and the user
  * is working with a copy: e.g. an xspf updater or a spotify subscribed playlist.
  */

class PlaylistUpdaterFactory;

// used when loading/saving from settings


class DLLEXPORT PlaylistUpdaterInterface : public QObject
{
    Q_OBJECT
public:
    explicit PlaylistUpdaterInterface( const playlist_ptr& pl );

    virtual ~PlaylistUpdaterInterface();

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

    /// If you want to try to load updaters for a playlist
    static void loadForPlaylist( const playlist_ptr& pl );

    static void registerUpdaterFactory( PlaylistUpdaterFactory* f );

    virtual bool sync() const { return false; }
    virtual void setSync( bool ) {}

    virtual bool canSubscribe() const { return false; }
    virtual bool subscribed() const { return false; }
    virtual void setSubscribed( bool ) {}

    // The int data value associated with each question must be unique across *all* playlist updaters,
    // as setQuestionResults is called with all questions from all updaters.
    virtual bool hasCustomDeleter() const { return false; }
    virtual PlaylistDeleteQuestions deleteQuestions() const { return PlaylistDeleteQuestions(); }
    virtual void setQuestionResults( const QMap< int, bool > results ) {}

signals:
    void changed();

public slots:
    virtual void updateNow() {}

    void save();

protected:
    virtual void aboutToDelete() {}

    QVariantHash settings() const;
    void saveSettings( const QVariantHash& settings );

private:
    playlist_ptr m_playlist;
    QVariantHash m_extraData;

    static QMap< QString, PlaylistUpdaterFactory* > s_factories;
};


class DLLEXPORT PlaylistUpdaterFactory
{
public:
    PlaylistUpdaterFactory() {}
    virtual ~PlaylistUpdaterFactory() {}

    virtual QString type() const = 0;
    virtual PlaylistUpdaterInterface* create( const playlist_ptr&, const QVariantHash& settings ) = 0;
};

}

Q_DECLARE_METATYPE( Tomahawk::SerializedUpdater );
Q_DECLARE_METATYPE( Tomahawk::SerializedUpdaters );

#endif // PLAYLISTUPDATERINTERFACE_H
