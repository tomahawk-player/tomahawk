/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 * 
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef TOMAHAWKSOURCEPLAYLISTINTERFACE_H
#define TOMAHAWKSOURCEPLAYLISTINTERFACE_H

#include <QObject>
#include <QSharedPointer>

#include "typedefs.h"
#include "collection.h"
#include "playlistinterface.h"

#include "dllmacro.h"

namespace Tomahawk
{

class DLLEXPORT SourcePlaylistInterface : public QObject, public PlaylistInterface
{
Q_OBJECT

public:
    SourcePlaylistInterface( Tomahawk::source_ptr& source );

    QList<Tomahawk::query_ptr> tracks();

    virtual int trackCount() const { return 1; }
    virtual int unfilteredTrackCount() const { return 1; }

    virtual Tomahawk::result_ptr siblingItem( int itemsAway );
    virtual Tomahawk::result_ptr nextItem();

    virtual PlaylistInterface::RepeatMode repeatMode() const { return PlaylistInterface::NoRepeat; }
    virtual PlaylistInterface::SkipSeekRestrictions skipSeekRestrictions() const { return PlaylistInterface::NoSkipSeek; }

    virtual bool shuffled() const { return false; }
    virtual void setFilter( const QString& /*pattern*/ ) {}

public slots:
    virtual void setRepeatMode( PlaylistInterface::RepeatMode ) {}
    virtual void setShuffled( bool ) {}

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );
    void trackCountChanged( unsigned int tracks );
    void sourceTrackCountChanged( unsigned int tracks );

private slots:
    void onSourcePlaybackStarted( const Tomahawk::query_ptr& query ) const;

private:
    Tomahawk::source_ptr m_source;
};

}; // ns

#endif
