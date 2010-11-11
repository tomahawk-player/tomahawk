#ifndef COLLECTIONFLATMODEL_H
#define COLLECTIONFLATMODEL_H

#include <QAbstractItemModel>
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

class CollectionFlatModel : public TrackModel
{
Q_OBJECT

public:
    explicit CollectionFlatModel( QObject* parent = 0 );
    ~CollectionFlatModel();

    int columnCount( const QModelIndex& parent = QModelIndex() ) const;

    QVariant data( const QModelIndex& index, int role ) const;
    QVariant headerData( int section, Qt::Orientation orientation, int role ) const;

    void addCollection( const Tomahawk::collection_ptr& collection );
    void removeCollection( const Tomahawk::collection_ptr& collection );

signals:
    void repeatModeChanged( PlaylistInterface::RepeatMode mode );
    void shuffleModeChanged( bool enabled );

    void itemSizeChanged( const QModelIndex& index );

    void loadingStarts();
    void loadingFinished();

private slots:
    void onDataChanged();

    void onTracksAdded( const QList<Tomahawk::query_ptr>& tracks, const Tomahawk::collection_ptr& collection );
    void onTracksAddingFinished( const Tomahawk::collection_ptr& collection );

    void onSourceOffline( const Tomahawk::source_ptr& src );

private:
    QMap< Tomahawk::collection_ptr, QPair< int, int > > m_collectionRows;
};

#endif // COLLECTIONFLATMODEL_H
