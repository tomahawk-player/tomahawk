#include "playlistmodel.h"

#include <QDebug>
#include <QMimeData>
#include <QTreeView>

using namespace Tomahawk;


PlaylistModel::PlaylistModel( QObject* parent )
    : TrackModel( parent )
{
    qDebug() << Q_FUNC_INFO;
}


PlaylistModel::~PlaylistModel()
{
    delete m_rootItem;
}


int
PlaylistModel::columnCount( const QModelIndex& parent ) const
{
    return TrackModel::columnCount( parent );
}


QVariant
PlaylistModel::data( const QModelIndex& index, int role ) const
{
    return TrackModel::data( index, role );
}


QVariant
PlaylistModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    return TrackModel::headerData( section, orientation, role );
}


void
PlaylistModel::loadPlaylist( const Tomahawk::playlist_ptr& playlist )
{
    m_playlist = playlist;
//    connect( playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), SLOT( onRevisionLoaded( Tomahawk::PlaylistRevision ) ) );

    PlItem* plitem;
    QList<plentry_ptr> entries = playlist->entries();
    int c = rowCount( QModelIndex() );

    qDebug() << "starting loading" << playlist->title();
    emit loadingStarts();
    emit beginInsertRows( QModelIndex(), c, c + entries.count() - 1 );

    foreach( const plentry_ptr& entry, entries )
    {
        plitem = new PlItem( entry, m_rootItem );
        plitem->index = createIndex( m_rootItem->children.count() - 1, 0, plitem );

        connect( plitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();

    qDebug() << rowCount( QModelIndex() );
    emit loadingFinished();
}


void
PlaylistModel::onDataChanged()
{
    PlItem* p = (PlItem*)sender();
//    emit itemSizeChanged( p->index );
    emit dataChanged( p->index, p->index.sibling( p->index.row(), columnCount() - 1 ) );
}
