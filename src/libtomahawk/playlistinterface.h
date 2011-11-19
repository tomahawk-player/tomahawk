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

#ifndef PLAYLISTINTERFACE_H
#define PLAYLISTINTERFACE_H

#include <QModelIndex>
#include <QWidget>

#include "typedefs.h"
#include "dllmacro.h"
#include "utils/logger.h"

namespace Tomahawk
{

class DLLEXPORT PlaylistInterface
{

public:
    enum RepeatMode { NoRepeat, RepeatOne, RepeatAll };
    Q_ENUMS( RepeatMode )
    enum ViewMode { Unknown, Tree, Flat, Album };
    enum SeekRestrictions { NoSeekRestrictions, NoSeek };
    enum SkipRestrictions { NoSkipRestrictions, NoSkipForwards, NoSkipBackwards, NoSkip };
    enum RetryMode { NoRetry, Retry };

    explicit PlaylistInterface( QObject* parent = 0 );
    virtual ~PlaylistInterface();

    virtual QList< Tomahawk::query_ptr > tracks() = 0;

    virtual int unfilteredTrackCount() const = 0;
    virtual int trackCount() const = 0;

    virtual Tomahawk::result_ptr currentItem() const = 0;
    virtual Tomahawk::result_ptr previousItem();
    virtual bool hasNextItem() { return true; }
    virtual Tomahawk::result_ptr nextItem();
    virtual Tomahawk::result_ptr siblingItem( int itemsAway ) = 0;

    virtual PlaylistInterface::RepeatMode repeatMode() const = 0;
    virtual bool shuffled() const = 0;
    virtual PlaylistInterface::ViewMode viewMode() const { return Unknown; }
    virtual PlaylistInterface::SeekRestrictions seekRestrictions() const { return NoSeekRestrictions; }
    virtual PlaylistInterface::SkipRestrictions skipRestrictions() const { return NoSkipRestrictions; }

    virtual PlaylistInterface::RetryMode retryMode() const { return NoRetry; }
    virtual quint32 retryInterval() const { return 30000; }

    virtual QString filter() const { return m_filter; }
    virtual void setFilter( const QString& pattern ) { m_filter = pattern; }

    virtual void reset() {}

    // Some playlist interfaces can wrap other interfaces. When checking for top-level
    // equality (say, to compare the currently playing interface) this might be needed
    virtual bool hasChildInterface( PlaylistInterface* ) { return false; }

    QObject* object() const { return m_object; }

    static void dontDelete( Tomahawk::PlaylistInterface* obj )
    {
        qDebug() << Q_FUNC_INFO << obj;
    }
    virtual Tomahawk::playlistinterface_ptr getSharedPointer()
    {
        if ( m_sharedPtr.isNull() )
        {
            m_sharedPtr = Tomahawk::playlistinterface_ptr( this, dontDelete );
        }

        return m_sharedPtr;
    }

public slots:
    virtual void setRepeatMode( RepeatMode mode ) = 0;
    virtual void setShuffled( bool enabled ) = 0;

signals:
    virtual void repeatModeChanged( PlaylistInterface::RepeatMode mode ) = 0;
    virtual void shuffleModeChanged( bool enabled ) = 0;
    virtual void trackCountChanged( unsigned int tracks ) = 0;
    virtual void sourceTrackCountChanged( unsigned int tracks ) = 0;
    virtual void nextTrackReady() = 0;

private:
    Q_DISABLE_COPY( PlaylistInterface )

    QObject* m_object;
    Tomahawk::playlistinterface_ptr m_sharedPtr;

    QString m_filter;
};

};

#endif // PLAYLISTINTERFACE_H
