#ifndef ALBUMMODEL_H
#define ALBUMMODEL_H

#include <QAbstractItemModel>
#include <QPixmap>

#include "tomahawk/album.h"
#include "tomahawk/collection.h"
#include "tomahawk/playlistinterface.h"
#include "database/databasecommand_allalbums.h"

#include "albumitem.h"

class QMetaData;

class AlbumModel : public QAbstractItemModel
{
Q_OBJECT

public:
    explicit AlbumModel( QObject* parent = 0 );
    virtual ~AlbumModel();

    virtual QModelIndex index( int row, int column, const QModelIndex& parent ) const;
    virtual QModelIndex parent( const QModelIndex& child ) const;

    virtual bool isReadOnly() const { return true; }

    virtual int trackCount() const { return rowCount( QModelIndex() ); }
    virtual int albumCount() const { return rowCount( QModelIndex() ); }

    virtual int rowCount( const QModelIndex& parent ) const;
    virtual int columnCount( const QModelIndex& parent ) const;

    virtual QVariant data( const QModelIndex& index, int role = Qt::DisplayRole ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role ) const;

    virtual void removeIndex( const QModelIndex& index );
    virtual void removeIndexes( const QList<QModelIndex>& indexes );

    virtual PlaylistInterface::RepeatMode repeatMode() const { return PlaylistInterface::NoRepeat; }
    virtual bool shuffled() const { return false; }

    virtual QMimeData* mimeData( const QModelIndexList& indexes ) const;
    virtual QStringList mimeTypes() const;
    virtual Qt::ItemFlags flags( const QModelIndex& index ) const;

    void addCollection( const Tomahawk::collection_ptr& collection );
    void addFilteredCollection( const Tomahawk::collection_ptr& collection, unsigned int amount, DatabaseCommand_AllAlbums::SortOrder order );

    AlbumItem* itemFromIndex( const QModelIndex& index ) const
    {
        if ( index.isValid() )
            return static_cast<AlbumItem*>( index.internalPointer() );
        else
        {
            return m_rootItem;
        }
    }

public slots:
    virtual void setRepeatMode( PlaylistInterface::RepeatMode mode ) {}
    virtual void setShuffled( bool shuffled ) {}

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void trackCountChanged( unsigned int tracks );

protected:

private slots:
    void onAlbumsAdded( const QList<Tomahawk::album_ptr>& albums, const Tomahawk::collection_ptr& collection );
    void onCoverArtDownloaded();
    void onDataChanged();

private:
    QPersistentModelIndex m_currentIndex;
    AlbumItem* m_rootItem;
    QPixmap m_defaultCover;
};

#endif // ALBUMMODEL_H
