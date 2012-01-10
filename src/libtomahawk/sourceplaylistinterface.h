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
#include "playlistinterface.h"

#include "dllmacro.h"

namespace Tomahawk
{

class DLLEXPORT SourcePlaylistInterface : public Tomahawk::PlaylistInterface
{
Q_OBJECT

public:
    SourcePlaylistInterface( Tomahawk::Source *source );
    virtual ~SourcePlaylistInterface();

    QList<Tomahawk::query_ptr> tracks();

    virtual int trackCount() const { return 1; }
    virtual int unfilteredTrackCount() const { return 1; }

    virtual Tomahawk::result_ptr siblingItem( int itemsAway );
    virtual bool hasNextItem();
    virtual Tomahawk::result_ptr nextItem();
    virtual Tomahawk::result_ptr currentItem() const;

    virtual PlaylistInterface::RepeatMode repeatMode() const { return PlaylistInterface::NoRepeat; }
    virtual PlaylistInterface::SeekRestrictions seekRestrictions() const { return PlaylistInterface::NoSeek; }
    virtual PlaylistInterface::SkipRestrictions skipRestrictions() const { return PlaylistInterface::NoSkipBackwards; }
    virtual PlaylistInterface::RetryMode retryMode() const { return Retry; }
    virtual quint32 retryInterval() const { return 5000; }

    virtual bool shuffled() const { return false; }
    virtual void setFilter( const QString& /*pattern*/ ) {}

    virtual QWeakPointer< Tomahawk::Source > source() const;

    virtual void reset();

public slots:
    virtual void setRepeatMode( PlaylistInterface::RepeatMode ) {}
    virtual void setShuffled( bool ) {}

private slots:
    void onSourcePlaybackStarted( const Tomahawk::query_ptr& query );
    void resolvingFinished( bool hasResults );

private:
    QWeakPointer< Tomahawk::Source > m_source;
    Tomahawk::result_ptr m_currentItem;
    bool m_gotNextItem;
};

}; // ns

#endif
