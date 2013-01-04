/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#ifndef TOMAHAWKALBUMPLAYLISTINTERFACE_H
#define TOMAHAWKALBUMPLAYLISTINTERFACE_H

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

#include "Album.h"
#include "Typedefs.h"
#include "PlaylistInterface.h"
#include "infosystem/InfoSystem.h"
#include "DllMacro.h"

namespace Tomahawk
{

class DLLEXPORT AlbumPlaylistInterface : public Tomahawk::PlaylistInterface
{
Q_OBJECT

public:
    AlbumPlaylistInterface( Tomahawk::Album* album, Tomahawk::ModelMode mode, const Tomahawk::collection_ptr& collection );
    virtual ~AlbumPlaylistInterface();

    QList<Tomahawk::query_ptr> tracks() const;

    virtual int trackCount() const { return m_queries.count(); }

    virtual void setCurrentIndex( qint64 index );
    virtual qint64 siblingIndex( int itemsAway, qint64 rootIndex = -1 ) const;

    virtual Tomahawk::result_ptr resultAt( qint64 index ) const;
    virtual Tomahawk::query_ptr queryAt( qint64 index ) const;
    virtual qint64 indexOfResult( const Tomahawk::result_ptr& result ) const;
    virtual qint64 indexOfQuery( const Tomahawk::query_ptr& query ) const;

    virtual Tomahawk::result_ptr currentItem() const;

    virtual PlaylistModes::RepeatMode repeatMode() const { return PlaylistModes::NoRepeat; }
    virtual bool shuffled() const { return false; }

    virtual void setRepeatMode( PlaylistModes::RepeatMode ) {}
    virtual void setShuffled( bool ) {}
    virtual bool setCurrentTrack( unsigned int albumpos );

signals:
    void tracksLoaded( Tomahawk::ModelMode mode, const Tomahawk::collection_ptr& collection );

private slots:
    void onTracksLoaded( const QList< Tomahawk::query_ptr >& tracks );
    void infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );
    void infoSystemFinished( const QString& infoId );

private:
    void checkQueries();

    QList< Tomahawk::query_ptr > m_queries;
    mutable result_ptr m_currentItem;

    bool m_infoSystemLoaded;
    bool m_databaseLoaded;

    Tomahawk::ModelMode m_mode;
    Tomahawk::collection_ptr m_collection;

    QWeakPointer< Tomahawk::Album > m_album;
};

}; // ns

#endif
