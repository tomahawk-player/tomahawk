#include "trackmodel.h"

#include <QDebug>
#include <QMimeData>
#include <QTreeView>

#include "audio/audioengine.h"
#include "utils/tomahawkutils.h"

#include "album.h"

using namespace Tomahawk;


TrackModel::TrackModel( QObject* parent )
    : QAbstractItemModel( parent )
    , m_rootItem( new PlItem( 0, this ) )
    , m_readOnly( true )
{
    qDebug() << Q_FUNC_INFO;

    connect( AudioEngine::instance(), SIGNAL( finished( Tomahawk::result_ptr ) ), SLOT( onPlaybackFinished( Tomahawk::result_ptr ) ), Qt::DirectConnection );
    connect( AudioEngine::instance(), SIGNAL( stopped() ), SLOT( onPlaybackStopped() ), Qt::DirectConnection );
}


TrackModel::~TrackModel()
{
    delete m_rootItem;
}


QModelIndex
TrackModel::index( int row, int column, const QModelIndex& parent ) const
{
    if ( !m_rootItem || row < 0 || column < 0 )
        return QModelIndex();

    PlItem* parentItem = itemFromIndex( parent );
    PlItem* childItem = parentItem->children.value( row );
    if ( !childItem )
        return QModelIndex();

    return createIndex( row, column, childItem );
}


int
TrackModel::rowCount( const QModelIndex& parent ) const
{
    if ( parent.column() > 0 )
        return 0;

    PlItem* parentItem = itemFromIndex( parent );
    if ( !parentItem )
        return 0;

    return parentItem->children.count();
}


int
TrackModel::columnCount( const QModelIndex& parent ) const
{
    return 9;
}


QModelIndex
TrackModel::parent( const QModelIndex& child ) const
{
    PlItem* entry = itemFromIndex( child );
    if ( !entry )
        return QModelIndex();

    PlItem* parentEntry = entry->parent;
    if ( !parentEntry )
        return QModelIndex();

    PlItem* grandparentEntry = parentEntry->parent;
    if ( !grandparentEntry )
        return QModelIndex();

    int row = grandparentEntry->children.indexOf( parentEntry );
    return createIndex( row, 0, parentEntry );
}


QVariant
TrackModel::data( const QModelIndex& index, int role ) const
{
    PlItem* entry = itemFromIndex( index );
    if ( !entry )
        return QVariant();

    if ( role == Qt::DecorationRole )
    {
        return QVariant();
    }

    if ( role == Qt::SizeHintRole )
    {
        return QSize( 0, 18 );
    }

    if ( role != Qt::DisplayRole ) // && role != Qt::ToolTipRole )
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
            case Artist:
                return query->artist();
                break;

            case Track:
                return query->track();
                break;

            case Album:
                return query->album();
                break;
        }
    }
    else
    {
        switch( index.column() )
        {
            case Artist:
                return query->results().first()->artist()->name();
                break;

            case Track:
                return query->results().first()->track();
                break;

            case Album:
                return query->results().first()->album()->name();
                break;

            case Duration:
                return TomahawkUtils::timeToString( query->results().first()->duration() );
                break;

            case Bitrate:
                return query->results().first()->bitrate();
                break;

            case Age:
                return TomahawkUtils::ageToString( QDateTime::fromTime_t( query->results().first()->modificationTime() ) );
                break;

            case Year:
                return query->results().first()->year();
                break;

            case Filesize:
                return TomahawkUtils::filesizeToString( query->results().first()->size() );
                break;

            case Origin:
                return query->results().first()->collection()->source()->friendlyName();
                break;
        }
    }

    return QVariant();
}


QVariant
TrackModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    QStringList headers;
    headers << tr( "Artist" ) << tr( "Track" ) << tr( "Album" ) << tr( "Duration" ) << tr( "Bitrate" ) << tr( "Age" ) << tr( "Year" ) << tr( "Size" ) << tr( "Origin" );
    if ( role == Qt::DisplayRole && section >= 0 )
    {
        return headers.at( section );
    }

    return QVariant();
}


void
TrackModel::setCurrentItem( const QModelIndex& index )
{
    qDebug() << Q_FUNC_INFO;
    PlItem* oldEntry = itemFromIndex( m_currentIndex );
    if ( oldEntry )
    {
        oldEntry->setIsPlaying( false );
    }

    PlItem* entry = itemFromIndex( index );
    if ( entry )
    {
        m_currentIndex = index;
        entry->setIsPlaying( true );
    }
    else
    {
        m_currentIndex = QModelIndex();
    }
}


Qt::DropActions
TrackModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


Qt::ItemFlags
TrackModel::flags( const QModelIndex& index ) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags( index );

    if ( index.isValid() && index.column() == 0 )
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    else
        return Qt::ItemIsDropEnabled | defaultFlags;
}


QStringList
TrackModel::mimeTypes() const
{
    QStringList types;
    types << "application/tomahawk.query.list";
    return types;
}


QMimeData*
TrackModel::mimeData( const QModelIndexList &indexes ) const
{
    qDebug() << Q_FUNC_INFO;

    QByteArray queryData;
    QDataStream queryStream( &queryData, QIODevice::WriteOnly );

    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 )
            continue;

        QModelIndex idx = index( i.row(), 0, i.parent() );
        PlItem* item = itemFromIndex( idx );
        if ( item )
        {
            const query_ptr& query = item->query();
            queryStream << qlonglong( &query );
        }
    }

    QMimeData* mimeData = new QMimeData();
    mimeData->setData( "application/tomahawk.query.list", queryData );

    return mimeData;
}


void
TrackModel::removeIndex( const QModelIndex& index, bool moreToCome )
{
    if ( QThread::currentThread() != thread() )
    {
        qDebug() << "Reinvoking in correct thread:" << Q_FUNC_INFO;
        QMetaObject::invokeMethod( this, "removeIndex",
                                   Qt::QueuedConnection,
                                   Q_ARG(const QModelIndex, index),
                                   Q_ARG(bool, moreToCome)
                                 );
        return;
    }

    qDebug() << Q_FUNC_INFO;

    if ( index.column() > 0 )
        return;

    PlItem* item = itemFromIndex( index );
    if ( item )
    {
        emit beginRemoveRows( index.parent(), index.row(), index.row() );
        delete item;
        emit endRemoveRows();
    }

    emit trackCountChanged( rowCount( QModelIndex() ) );
}


void
TrackModel::removeIndexes( const QList<QModelIndex>& indexes )
{
    foreach( const QModelIndex& idx, indexes )
    {
        removeIndex( idx );
    }
}


PlItem*
TrackModel::itemFromIndex( const QModelIndex& index ) const
{
    if ( index.isValid() )
        return static_cast<PlItem*>( index.internalPointer() );
    else
    {
        return m_rootItem;
    }
}


void
TrackModel::onPlaybackFinished( const Tomahawk::result_ptr& result )
{
    PlItem* oldEntry = itemFromIndex( m_currentIndex );
    if ( oldEntry && !oldEntry->query().isNull() && oldEntry->query()->results().contains( result ) )
    {
        oldEntry->setIsPlaying( false );
    }
}


void
TrackModel::onPlaybackStopped()
{
    PlItem* oldEntry = itemFromIndex( m_currentIndex );
    if ( oldEntry )
    {
        oldEntry->setIsPlaying( false );
    }
}
