#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QList>
#include <QHash>

#include "plitem.h"
#include "trackmodel.h"
#include "tomahawk/tomahawkapp.h"
#include "tomahawk/collection.h"
#include "tomahawk/query.h"
#include "tomahawk/typedefs.h"
#include "tomahawk/playlist.h"
#include "tomahawk/playlistinterface.h"

class QMetaData;

class PlaylistModel : public TrackModel
{
Q_OBJECT

public:
    explicit PlaylistModel( QObject* parent = 0 );
    ~PlaylistModel();

    int columnCount( const QModelIndex& parent = QModelIndex() ) const;

    QVariant data( const QModelIndex& index, int role ) const;
    QVariant headerData( int section, Qt::Orientation orientation, int role ) const;

    virtual bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent );

    void loadPlaylist( const Tomahawk::playlist_ptr& playlist );
    void loadAlbum( const Tomahawk::album_ptr& album );

    void appendTrack( const Tomahawk::query_ptr& query );

    virtual void removeIndex( const QModelIndex& index );

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void itemSizeChanged( const QModelIndex& index );

    void loadingStarts();
    void loadingFinished();

private slots:
    void onDataChanged();

    void onRevisionLoaded( Tomahawk::PlaylistRevision revision );
    void onPlaylistChanged();

    void onTracksAdded( const QList<Tomahawk::query_ptr>& tracks, const Tomahawk::collection_ptr& collection = Tomahawk::collection_ptr() );

private:
    QList<Tomahawk::plentry_ptr> playlistEntries() const;

    Tomahawk::playlist_ptr m_playlist;
    bool m_waitForUpdate;
};

#endif // PLAYLISTMODEL_H
