#ifndef PLAYLISTMODEL_H
#define PLAYLISTMODEL_H

#include <QStandardItemModel>
#include <QList>
#include <QThread>
#include <QTimer>

#include "playlistitem.h"
#include "tomahawk/tomahawkapp.h"
#include "tomahawk/collection.h"
#include "tomahawk/query.h"
#include "tomahawk/typedefs.h"
#include "tomahawk/playlist.h"
#include "tomahawk/playlistmodelinterface.h"

class QMetaData;

class PlaylistModel : public QStandardItemModel, public PlaylistModelInterface
{
Q_OBJECT
friend class PlaylistModelWorker;

public:
    explicit PlaylistModel( QObject* parent = 0 );

    virtual QMimeData* mimeData( const QModelIndexList& indexes ) const;
    virtual QStringList mimeTypes() const;
    virtual Qt::DropActions supportedDropActions() const;
    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;
    virtual bool dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent );

    bool isReadOnly() const { return m_readOnly; }
    bool isBusy() const { return m_busy; }

    static int indexType( const QModelIndex& index );
    static PlaylistItem* indexToPlaylistItem( const QModelIndex& index );

    bool appendItem( Tomahawk::query_ptr query, bool emitsig = true );
    bool insertItems( int pos, const QList<PlaylistItem*>& items, bool emitsig = true );
    void removeItems( const QList<PlaylistItem*>& items, bool emitsig = true, bool animated = false );

    virtual void loadPlaylist( const Tomahawk::playlist_ptr& playlist );
    bool isPlaylistBacked() const { return !m_playlist.isNull(); }
    const Tomahawk::playlist_ptr& playlist() const { return m_playlist; }
    QList<Tomahawk::plentry_ptr> const playlistEntries();

    void addCollection( const Tomahawk::collection_ptr& collection );
    void removeCollection( const Tomahawk::collection_ptr& collection );

    virtual void addSource( const Tomahawk::source_ptr& source );
    virtual void removeSource( const Tomahawk::source_ptr& source );

    virtual void setCurrentItem( const QModelIndex& index );
    virtual PlaylistItem* previousItem();
    virtual PlaylistItem* nextItem();
    virtual PlaylistItem* siblingItem( int itemsAway );

    virtual unsigned int sourceCount() { return m_sourceCount; }
    virtual unsigned int collectionCount() { return m_collectionCount; }
    virtual unsigned int trackCount();
    virtual unsigned int artistCount() { return m_artists.size(); }
    void updateStats();

    QPersistentModelIndex currentItem() const { return m_currentItem; }

public slots:
    void setReadOnly( bool readOnly ) { m_readOnly = readOnly; }
    virtual void setRepeatMode( RepeatMode mode ) { qWarning() << Q_FUNC_INFO << "This should never get called directly!"; }
    virtual void setShuffled( bool enabled ) { qWarning() << Q_FUNC_INFO << "This should never get called directly!"; }

    bool appendItems( const QList<PlaylistItem*>& items, bool emitsig = true );
    void onTracksAdded( const QList<QVariant>& tracks, Tomahawk::collection_ptr collection );
    void onTracksRemoved( const QList<QVariant>& tracks, Tomahawk::collection_ptr collection );

signals:
    //void sourceCountChanged( unsigned int counter );
    void collectionCountChanged( unsigned int counter );
    //void trackCountChanged( unsigned int counter );

    void numSourcesChanged( unsigned int i );
    void numTracksChanged( unsigned int i );
    void numArtistsChanged( unsigned int i );
    void numShownChanged( unsigned int i );

    void repeatModeChanged( PlaylistModelInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

private slots:
    void workerFinished();
    void updateInternalPlaylist();
    void onRevisionLoaded( Tomahawk::PlaylistRevision revision );

    void onRowsAboutToBeRemoved( const QModelIndex& parent, int start, int end );
    void onRowsInserted( const QModelIndex& parent, int start, int end );
    void onRowsRemoved( const QModelIndex& parent, int start, int end );

    void onRowRemoverFinished() { m_busy = false; }

private:
    void setupHeaders();
    void appendTracks( const QList<Tomahawk::query_ptr>& queries, Tomahawk::collection_ptr collection );
    void appendPlaylistEntries( const QList<Tomahawk::plentry_ptr>& entries, Tomahawk::collection_ptr collection );

    Tomahawk::playlist_ptr m_playlist;

    unsigned int m_collectionCount;
    unsigned int m_sourceCount;

    bool m_remoteUpdate;
    bool m_readOnly;
    QIcon m_nowPlayingIcon;

    mutable Tomahawk::plentry_ptr m_currentPlEntry;
    QPersistentModelIndex m_currentItem;
    QMap<QString, unsigned int> m_artists;
    unsigned int m_sources;
    bool m_busy;
};

#endif // PLAYLISTMODEL_H
