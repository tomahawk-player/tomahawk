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

#ifndef TOMAHAWKARTISTPLAYLISTINTERFACE_H
#define TOMAHAWKARTISTPLAYLISTINTERFACE_H

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

#include "Artist.h"
#include "Typedefs.h"
#include "PlaylistInterface.h"
#include "DllMacro.h"

namespace Tomahawk
{

class DLLEXPORT ArtistPlaylistInterface : public Tomahawk::PlaylistInterface
{
Q_OBJECT

public:
    ArtistPlaylistInterface( Tomahawk::Artist* artist, Tomahawk::ModelMode mode, const Tomahawk::collection_ptr& collection );
    virtual ~ArtistPlaylistInterface();

    virtual QList<Tomahawk::query_ptr> tracks();

    virtual int trackCount() const { return m_queries.count(); }

    virtual Tomahawk::result_ptr siblingItem( int itemsAway );

    virtual bool hasNextItem();
    virtual Tomahawk::result_ptr currentItem() const;

    virtual PlaylistModes::RepeatMode repeatMode() const { return PlaylistModes::NoRepeat; }
    virtual bool shuffled() const { return false; }

    virtual void setRepeatMode( PlaylistModes::RepeatMode ) {}
    virtual void setShuffled( bool ) {}

signals:
    void tracksLoaded( Tomahawk::ModelMode mode, const Tomahawk::collection_ptr& collection );

private slots:
    void onTracksLoaded( const QList< Tomahawk::query_ptr >& tracks );
    void infoSystemInfo( Tomahawk::InfoSystem::InfoRequestData requestData, QVariant output );
    void infoSystemFinished( const QString& infoId );

private:
    Q_DISABLE_COPY( ArtistPlaylistInterface )

    QList< Tomahawk::query_ptr > m_queries;
    result_ptr m_currentItem;
    unsigned int m_currentTrack;

    bool m_infoSystemLoaded;
    bool m_databaseLoaded;

    Tomahawk::ModelMode m_mode;
    Tomahawk::collection_ptr m_collection;

    QWeakPointer< Tomahawk::Artist > m_artist;
};

}; // ns

#endif
