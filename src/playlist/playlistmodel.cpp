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
    return 6;
}


QVariant
PlaylistModel::data( const QModelIndex& index, int role ) const
{
    PlItem* entry = itemFromIndex( index );
    if ( !entry )
        return QVariant();

    if ( role == Qt::DecorationRole )
    {
        if ( index.column() == 0 && entry->isPlaying() )
            return QString( RESPATH "images/now-playing-speaker.png" );

        return QVariant();
    }

    if ( role == Qt::SizeHintRole )
    {
        return QSize( 0, 18 );
    }

    if ( role != Qt::DisplayRole )
        return QVariant();

    const query_ptr& query = entry->query();
    if ( query.isNull() )
    {
        if ( !index.column() )
        {
            return entry->caption.isEmpty() ? "Unknown" : entry->caption;
        }

        if ( index.column() == 1 )
        {
            return entry->childCount;
        }

        return QVariant( "" );
    }

    if ( !query->numResults() )
    {
        switch( index.column() )
        {
            case 0:
                return query->artist();
                break;

            case 1:
                return query->track();
                break;

            case 2:
                return query->album();
                break;
        }
    }
    else
    {
        switch( index.column() )
        {
            case 0:
                return query->results().first()->artist();
                break;

            case 1:
                return query->results().first()->track();
                break;

            case 2:
                return query->results().first()->album();
                break;

            case 3:
                return TomahawkUtils::timeToString( query->results().first()->duration() );
                break;

            case 4:
                return query->results().first()->bitrate();
                break;

            case 5:
                return query->results().first()->collection()->source()->friendlyName();
                break;
        }
    }

    return QVariant();
}


QVariant
PlaylistModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    QStringList headers;
    headers << tr( "Artist" ) << tr( "Track" ) << tr( "Album" ) << tr( "Duration" ) << tr( "Bitrate" ) << tr( "Origin" );
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole && section >= 0 )
    {
        return headers.at( section );
    }

    return QVariant();
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
