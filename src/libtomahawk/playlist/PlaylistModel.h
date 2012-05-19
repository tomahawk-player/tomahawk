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

#include "Typedefs.h"
#include "TrackModel.h"
#include "Playlist.h"
#include "Query.h"
#include "PlaylistInterface.h"

#include "DllMacro.h"

class QMimeData;
class QMetaData;

class DLLEXPORT PlaylistModel : public TrackModel
{
Q_OBJECT

typedef struct {
    int row;
    QPersistentModelIndex parent;
    Qt::DropAction action;
} DropStorageData;

public:
    explicit PlaylistModel( QObject* parent = 0 );
    ~PlaylistModel();

    virtual QMimeData* mimeData( const QModelIndexList& indexes ) const;
    virtual bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent );

    Tomahawk::playlist_ptr playlist() const { return m_playlist; }

    virtual void loadPlaylist( const Tomahawk::playlist_ptr& playlist, bool loadEntries = true );
    bool isTemporary() const;

public slots:
    virtual void clear();

    virtual void append( const Tomahawk::query_ptr& query );
    virtual void append( const Tomahawk::album_ptr& album );
    virtual void append( const Tomahawk::artist_ptr& artist );
    virtual void append( const QList< Tomahawk::query_ptr >& queries );
    virtual void append( const QList< Tomahawk::plentry_ptr >& entries );

    virtual void insert( const Tomahawk::query_ptr& query, int row = 0 );
    virtual void insert( const QList< Tomahawk::query_ptr >& queries, int row = 0 );
    virtual void insert( const QList< Tomahawk::plentry_ptr >& entries, int row = 0 );

    virtual void remove( int row, bool moreToCome = false );
    virtual void remove( const QModelIndex& index, bool moreToCome = false );
    virtual void remove( const QList<QModelIndex>& indexes );
    virtual void remove( const QList<QPersistentModelIndex>& indexes );

signals:
    void repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode mode );
    void shuffleModeChanged( bool enabled );
    void playlistDeleted();
    void playlistChanged();

protected:
    bool waitForRevision( const QString& revisionguid ) const { return m_waitForRevision.contains( revisionguid ); }
    void removeFromWaitList( const QString& revisionguid ) { m_waitForRevision.removeAll( revisionguid ); }

private slots:
    void onRevisionLoaded( Tomahawk::PlaylistRevision revision );
    void parsedDroppedTracks( QList<Tomahawk::query_ptr> );
    void trackResolved( bool );

private:
    void beginPlaylistChanges();
    void endPlaylistChanges();

    QList<Tomahawk::plentry_ptr> playlistEntries() const;

    Tomahawk::playlist_ptr m_playlist;
    bool m_isTemporary;
    bool m_changesOngoing;
    bool m_isLoading;
    QList< Tomahawk::Query* > m_waitingForResolved;
    QStringList m_waitForRevision;

    int m_savedInsertPos;
    QList< Tomahawk::plentry_ptr > m_savedInsertTracks;
    QList< Tomahawk::query_ptr > m_savedRemoveTracks;

    DropStorageData m_dropStorage;
};

#endif // PLAYLISTMODEL_H
