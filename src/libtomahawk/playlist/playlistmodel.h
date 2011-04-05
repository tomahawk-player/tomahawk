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

#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QList>
#include <QHash>

#include "plitem.h"
#include "trackmodel.h"
#include "collection.h"
#include "query.h"
#include "typedefs.h"
#include "playlist.h"
#include "playlistinterface.h"

#include "dllmacro.h"

class QMetaData;

class DLLEXPORT PlaylistModel : public TrackModel
{
Q_OBJECT

public:
    explicit PlaylistModel( QObject* parent = 0 );
    ~PlaylistModel();

    int columnCount( const QModelIndex& parent = QModelIndex() ) const;

    QVariant data( const QModelIndex& index, int role ) const;
    QVariant headerData( int section, Qt::Orientation orientation, int role ) const;

    virtual bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent );

    Tomahawk::playlist_ptr playlist() const { return m_playlist; }

    virtual void loadPlaylist( const Tomahawk::playlist_ptr& playlist, bool loadEntries = true );
    void loadHistory( const Tomahawk::source_ptr& source, unsigned int amount = 50 );

    void clear();

    void append( const Tomahawk::query_ptr& query );
    void append( const Tomahawk::album_ptr& album );
    void append( const Tomahawk::artist_ptr& artist );

    void insert( unsigned int row, const Tomahawk::query_ptr& query );

    void remove( unsigned int row, bool moreToCome = false );
    virtual void removeIndex( const QModelIndex& index, bool moreToCome = false );

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void itemSizeChanged( const QModelIndex& index );
private slots:
    void onDataChanged();

    void onRevisionLoaded( Tomahawk::PlaylistRevision revision );
    void onPlaylistChanged( bool waitForUpdate = true );

    void onTracksAdded( const QList<Tomahawk::query_ptr>& tracks );
    void onTracksInserted( unsigned int row, const QList<Tomahawk::query_ptr>& tracks );

    void trackResolved( bool );
private:
    QList<Tomahawk::plentry_ptr> playlistEntries() const;

    Tomahawk::playlist_ptr m_playlist;
    bool m_waitForUpdate;
    QList< Tomahawk::Query* > m_waitingForResolved;
};

#endif // PLAYLISTMODEL_H
