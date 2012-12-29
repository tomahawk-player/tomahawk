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
#include "PlayableModel.h"
#include "Playlist.h"
#include "Query.h"
#include "PlaylistInterface.h"

#include "DllMacro.h"

class QMimeData;
class QMetaData;

class DLLEXPORT PlaylistModel : public PlayableModel
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

    virtual QString guid() const;

    virtual QMimeData* mimeData( const QModelIndexList& indexes ) const;
    virtual bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent );

    Tomahawk::playlist_ptr playlist() const { return m_playlist; }

    virtual void loadPlaylist( const Tomahawk::playlist_ptr& playlist, bool loadEntries = true );
    bool isTemporary() const;

    bool acceptPlayableQueriesOnly() const { return m_acceptPlayableQueriesOnly; }
    void setAcceptPlayableQueriesOnly( bool b ) { m_acceptPlayableQueriesOnly = b; }

public slots:
    virtual void clear();

    virtual void appendEntries( const QList< Tomahawk::plentry_ptr >& entries );

    virtual void insertAlbums( const QList< Tomahawk::album_ptr >& album, int row = 0 );
    virtual void insertArtists( const QList< Tomahawk::artist_ptr >& artist, int row = 0 );
    virtual void insertQueries( const QList< Tomahawk::query_ptr >& queries, int row = 0 );
    virtual void insertEntries( const QList< Tomahawk::plentry_ptr >& entries, int row = 0 );

    virtual void removeIndex( const QModelIndex& index, bool moreToCome = false );

signals:
    void repeatModeChanged( Tomahawk::PlaylistModes::RepeatMode mode );
    void shuffleModeChanged( bool enabled );
    void playlistDeleted();
    void playlistChanged();

protected:
    bool waitForRevision( const QString& revisionguid ) const { return m_waitForRevision.contains( revisionguid ); }
    void removeFromWaitList( const QString& revisionguid ) { m_waitForRevision.removeAll( revisionguid ); }

    QList<Tomahawk::plentry_ptr> playlistEntries() const;

private slots:
    void onRevisionLoaded( Tomahawk::PlaylistRevision revision );
    void parsedDroppedTracks( QList<Tomahawk::query_ptr> );
    void trackResolved( bool );
    void onPlaylistChanged();

private:
    void beginPlaylistChanges();
    void endPlaylistChanges();

    Tomahawk::playlist_ptr m_playlist;
    bool m_isTemporary;
    bool m_changesOngoing;
    bool m_isLoading;
    bool m_acceptPlayableQueriesOnly;
    QList< Tomahawk::Query* > m_waitingForResolved;
    QStringList m_waitForRevision;

    int m_savedInsertPos;
    QList< Tomahawk::plentry_ptr > m_savedInsertTracks;
    QList< Tomahawk::query_ptr > m_savedRemoveTracks;

    DropStorageData m_dropStorage;
};

#endif // PLAYLISTMODEL_H
