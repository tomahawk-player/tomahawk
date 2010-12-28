#ifndef COLLECTIONMODEL_H
#define COLLECTIONMODEL_H

#include <QAbstractItemModel>
#include <QList>
#include <QHash>

#include "plitem.h"
#include "collection.h"
#include "query.h"
#include "typedefs.h"
#include "playlist.h"
#include "playlistinterface.h"

class QMetaData;

class CollectionModel : public QAbstractItemModel
{
Q_OBJECT

public:
    explicit CollectionModel( QObject* parent = 0 );
    ~CollectionModel();

    QModelIndex index( int row, int column, const QModelIndex& parent ) const;
    QModelIndex parent( const QModelIndex& child ) const;

    int trackCount() const { return rowCount( QModelIndex() ); }

    int rowCount( const QModelIndex& parent ) const;
    int columnCount( const QModelIndex& parent ) const;

    QVariant data( const QModelIndex& index, int role ) const;
    QVariant headerData( int section, Qt::Orientation orientation, int role ) const;

    void addCollection( const Tomahawk::collection_ptr& collection );
    void removeCollection( const Tomahawk::collection_ptr& collection );

    virtual PlaylistInterface::RepeatMode repeatMode() const { return PlaylistInterface::NoRepeat; }
    virtual bool shuffled() const { return false; }

    virtual void setRepeatMode( PlaylistInterface::RepeatMode mode ) {}
    virtual void setShuffled( bool shuffled ) {}

    PlItem* itemFromIndex( const QModelIndex& index ) const;

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void loadingStarts();
    void loadingFinished();
    void trackCountChanged( unsigned int tracks );

private slots:
    void onTracksAdded( const QList<QVariant>& tracks, const Tomahawk::collection_ptr& collection );
    void onTracksAddingFinished( const Tomahawk::collection_ptr& collection );

    void onSourceOffline( Tomahawk::source_ptr src );

private:
    PlItem* m_rootItem;
    QMap< Tomahawk::collection_ptr, PlItem* > m_collectionIndex;
};

#endif // COLLECTIONMODEL_H
