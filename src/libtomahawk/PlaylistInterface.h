/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2012, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#ifndef PLAYLISTINTERFACE_H
#define PLAYLISTINTERFACE_H

#include <QtCore/QModelIndex>

#include "playlist/PlayableItem.h"
#include "Typedefs.h"
#include "DllMacro.h"

namespace Tomahawk
{

class DLLEXPORT PlaylistInterface : public QObject
{
Q_OBJECT

public:
    explicit PlaylistInterface();
    virtual ~PlaylistInterface();

    const QString id() const { return m_id; }

    virtual QList< Tomahawk::query_ptr > tracks() const = 0;
    virtual bool isFinished() const { return m_finished; }

    virtual int trackCount() const = 0;

    virtual Tomahawk::result_ptr currentItem() const = 0;
    virtual void setCurrentIndex( qint64 index );

    virtual bool hasNextResult() const;
    virtual bool hasPreviousResult() const;
    virtual Tomahawk::result_ptr nextResult() const;
    virtual Tomahawk::result_ptr previousResult() const;

    virtual qint64 siblingIndex( int itemsAway, qint64 rootIndex = -1 ) const = 0;
    virtual qint64 siblingResultIndex( int itemsAway, qint64 rootIndex = -1 ) const;
    virtual Tomahawk::result_ptr siblingResult( int itemsAway, qint64 rootIndex = -1 ) const;
    virtual Tomahawk::result_ptr setSiblingResult( int itemsAway, qint64 rootIndex = -1 );
    
    virtual Tomahawk::result_ptr resultAt( qint64 index ) const = 0;
    virtual Tomahawk::query_ptr queryAt( qint64 index ) const = 0;
    virtual qint64 indexOfResult( const Tomahawk::result_ptr& result ) const = 0;
    virtual qint64 indexOfQuery( const Tomahawk::query_ptr& query ) const = 0;

    virtual int posOfResult( const Tomahawk::result_ptr& result ) const;
    virtual int posOfQuery( const Tomahawk::query_ptr& query ) const;

    virtual PlaylistModes::RepeatMode repeatMode() const = 0;
    virtual bool shuffled() const = 0;

    virtual PlaylistModes::ViewMode viewMode() const { return PlaylistModes::Unknown; }

    virtual PlaylistModes::SeekRestrictions seekRestrictions() const { return PlaylistModes::NoSeekRestrictions; }
    virtual PlaylistModes::SkipRestrictions skipRestrictions() const { return PlaylistModes::NoSkipRestrictions; }

    virtual PlaylistModes::RetryMode retryMode() const { return PlaylistModes::NoRetry; }
    virtual quint32 retryInterval() const { return 30000; }

    virtual PlaylistModes::LatchMode latchMode() const { return m_latchMode; }
    virtual void setLatchMode( PlaylistModes::LatchMode latchMode ) { m_latchMode = latchMode; }

    virtual bool setCurrentTrack( unsigned int albumpos ) { Q_UNUSED( albumpos ); return false; }

    virtual void reset() {}

    //TODO: Get rid of the next two functions once all playlsitinterfaces are factored out
    // Some playlist interfaces can wrap other interfaces. When checking for top-level
    // equality (say, to compare the currently playing interface) this might be needed
    virtual bool hasChildInterface( const Tomahawk::playlistinterface_ptr& ) { return false; }

public slots:
    virtual void setRepeatMode( PlaylistModes::RepeatMode mode ) = 0;
    virtual void setShuffled( bool enabled ) = 0;

signals:
    void itemCountChanged( unsigned int tracks );
    void repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode mode );
    void shuffleModeChanged( bool enabled );
    void latchModeChanged( Tomahawk::PlaylistModes::LatchMode mode );

    void previousTrackAvailable( bool available );
    void nextTrackAvailable( bool available );

    void currentIndexChanged();

protected slots:
    virtual void onItemsChanged();

protected:
    virtual QList<Tomahawk::query_ptr> filterTracks( const QList<Tomahawk::query_ptr>& queries );

    PlaylistModes::LatchMode m_latchMode;
    bool m_finished;
    mutable bool m_prevAvail;
    mutable bool m_nextAvail;
    mutable qint64 m_currentIndex;

private:
    Q_DISABLE_COPY( PlaylistInterface )

private:
    QString m_id;
    QString m_filter;
};

}

Q_DECLARE_METATYPE( Tomahawk::playlistinterface_ptr )

#endif // PLAYLISTINTERFACE_H
