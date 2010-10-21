#ifndef COLLECTIONMODEL_H
#define COLLECTIONMODEL_H

#include <QAbstractItemModel>
#include <QList>
#include <QHash>

#include "plitem.h"
#include "tomahawk/tomahawkapp.h"
#include "tomahawk/collection.h"
#include "tomahawk/query.h"
#include "tomahawk/typedefs.h"
#include "tomahawk/playlist.h"
#include "tomahawk/playlistinterface.h"

class QMetaData;

class CollectionModel : public QAbstractItemModel, public PlaylistInterface
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

//    PlItem* itemFromIndex( const QModelIndex& index ) const;

    virtual PlItem* previousItem() { return 0; }
    virtual PlItem* nextItem() { return 0; }
    virtual PlItem* siblingItem( int direction ) { return 0; }

    virtual void setCurrentItem( const QModelIndex& index ) {}

    virtual PlaylistInterface::RepeatMode repeatMode() const { return PlaylistInterface::NoRepeat; }
    virtual bool shuffled() const { return false; }

    virtual void setRepeatMode( PlaylistInterface::RepeatMode mode ) {}
    virtual void setShuffled( bool shuffled ) {}

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void loadingStarts();
    void loadingFinished();
    void trackCountChanged( unsigned int tracks );

private slots:
    void onTracksAdded( const QList<QVariant>& tracks, const Tomahawk::collection_ptr& collection );
    void onTracksAddingFinished( const Tomahawk::collection_ptr& collection );

private:
    QMap< Tomahawk::collection_ptr, PlItem* > m_collectionIndex;
};

#endif // COLLECTIONMODEL_H
