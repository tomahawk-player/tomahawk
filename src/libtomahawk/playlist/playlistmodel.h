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

    void loadPlaylist( const Tomahawk::playlist_ptr& playlist, bool loadEntries = true );
    void loadHistory( const Tomahawk::source_ptr& source, unsigned int amount = 100 );

    void clear();
    
    void append( const Tomahawk::query_ptr& query );
    void append( const Tomahawk::album_ptr& album );
    void append( const Tomahawk::artist_ptr& artist );

    void insert( unsigned int row, const Tomahawk::query_ptr& query );

    virtual void removeIndex( const QModelIndex& index, bool moreToCome = false );

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void itemSizeChanged( const QModelIndex& index );

    void loadingStarts();
    void loadingFinished();

private slots:
    void onDataChanged();

    void onRevisionLoaded( Tomahawk::PlaylistRevision revision );
    void onPlaylistChanged( bool waitForUpdate = true );

    void onTracksAdded( const QList<Tomahawk::query_ptr>& tracks, const Tomahawk::collection_ptr& collection = Tomahawk::collection_ptr() );
    void onTracksInserted( unsigned int row, const QList<Tomahawk::query_ptr>& tracks, const Tomahawk::collection_ptr& collection = Tomahawk::collection_ptr() );

private:
    QList<Tomahawk::plentry_ptr> playlistEntries() const;

    Tomahawk::playlist_ptr m_playlist;
    bool m_waitForUpdate;
};

#endif // PLAYLISTMODEL_H
