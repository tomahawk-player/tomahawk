#include "albummodel.h"

#include <QDebug>
#include <QListView>
#include <QMimeData>
#include <QNetworkReply>

#include "tomahawk/tomahawkapp.h"
#include "database/database.h"

#define LASTFM_DEFAULT_COVER "http://cdn.last.fm/flatness/catalogue/noimage"

using namespace Tomahawk;


AlbumModel::AlbumModel( QObject* parent )
    : QAbstractItemModel( parent )
    , m_rootItem( new AlbumItem( 0, this ) )
{
    qDebug() << Q_FUNC_INFO;

    m_defaultCover = QPixmap( RESPATH "images/no-album-art-placeholder.png" )
                     .scaled( QSize( 120, 120 ), Qt::IgnoreAspectRatio, Qt::SmoothTransformation );
}


AlbumModel::~AlbumModel()
{
    delete m_rootItem;
}


QModelIndex
AlbumModel::index( int row, int column, const QModelIndex& parent ) const
{
    if ( !m_rootItem || row < 0 || column < 0 )
        return QModelIndex();

    AlbumItem* parentItem = itemFromIndex( parent );
    AlbumItem* childItem = parentItem->children.value( row );
    if ( !childItem )
        return QModelIndex();

    return createIndex( row, column, childItem );
}


int
AlbumModel::rowCount( const QModelIndex& parent ) const
{
    if ( parent.column() > 0 )
        return 0;

    AlbumItem* parentItem = itemFromIndex( parent );
    if ( !parentItem )
        return 0;

    return parentItem->children.count();
}


int
AlbumModel::columnCount( const QModelIndex& parent ) const
{
    return 1;
}


QModelIndex
AlbumModel::parent( const QModelIndex& child ) const
{
    AlbumItem* entry = itemFromIndex( child );
    if ( !entry )
        return QModelIndex();

    AlbumItem* parentEntry = entry->parent;
    if ( !parentEntry )
        return QModelIndex();

    AlbumItem* grandparentEntry = parentEntry->parent;
    if ( !grandparentEntry )
        return QModelIndex();

    int row = grandparentEntry->children.indexOf( parentEntry );
    return createIndex( row, 0, parentEntry );
}


QVariant
AlbumModel::data( const QModelIndex& index, int role ) const
{
    AlbumItem* entry = itemFromIndex( index );
    if ( !entry )
        return QVariant();

    if ( role == Qt::DecorationRole )
    {
        return entry->cover;
    }

    if ( role == Qt::SizeHintRole )
    {
        return QSize( 116, 150 );
    }

    if ( role != Qt::DisplayRole ) // && role != Qt::ToolTipRole )
        return QVariant();

    const album_ptr& album = entry->album();
    switch( index.column() )
    {
        case 0:
            return album->name() + "<br/>Test\nTest2";
            break;

    }

    return QVariant();
}


QVariant
AlbumModel::headerData( int section, Qt::Orientation orientation, int role ) const
{
    QStringList headers;
    headers << tr( "Album" );
    if ( orientation == Qt::Horizontal && role == Qt::DisplayRole && section >= 0 )
    {
        return headers.at( section );
    }

    return QVariant();
}


Qt::ItemFlags
AlbumModel::flags( const QModelIndex& index ) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags( index );

    if ( index.isValid() && index.column() == 0 )
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    else
        return defaultFlags;
}


QStringList
AlbumModel::mimeTypes() const
{
    QStringList types;
    types << "application/tomahawk.query.list";
    return types;
}


QMimeData*
AlbumModel::mimeData( const QModelIndexList &indexes ) const
{
    qDebug() << Q_FUNC_INFO;

    QByteArray queryData;
    QDataStream queryStream( &queryData, QIODevice::WriteOnly );

    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 )
            continue;

        QModelIndex idx = index( i.row(), 0, i.parent() );
        AlbumItem* item = itemFromIndex( idx );
        if ( item )
        {
            const album_ptr& album = item->album();
            queryStream << qlonglong( &album );
        }
    }

    QMimeData* mimeData = new QMimeData();
    mimeData->setData( "application/tomahawk.query.list", queryData );

    return mimeData;
}


void
AlbumModel::removeIndex( const QModelIndex& index )
{
    qDebug() << Q_FUNC_INFO;

    if ( index.column() > 0 )
        return;

    AlbumItem* item = itemFromIndex( index );
    if ( item )
    {
        emit beginRemoveRows( index.parent(), index.row(), index.row() );
        delete item;
        emit endRemoveRows();
    }
}


void
AlbumModel::removeIndexes( const QList<QModelIndex>& indexes )
{
    foreach( const QModelIndex& idx, indexes )
    {
        removeIndex( idx );
    }
}


void
AlbumModel::addCollection( const collection_ptr& collection )
{
    qDebug() << Q_FUNC_INFO << collection->name()
                            << collection->source()->id()
                            << collection->source()->userName();

    DatabaseCommand_AllAlbums* cmd = new DatabaseCommand_AllAlbums( collection );

    connect( cmd, SIGNAL( albums( QList<Tomahawk::album_ptr>, Tomahawk::collection_ptr ) ),
                    SLOT( onAlbumsAdded( QList<Tomahawk::album_ptr>, Tomahawk::collection_ptr ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
AlbumModel::addFilteredCollection( const collection_ptr& collection, unsigned int amount, DatabaseCommand_AllAlbums::SortOrder order )
{
    qDebug() << Q_FUNC_INFO << collection->name()
                            << collection->source()->id()
                            << collection->source()->userName()
                            << amount << order;

    DatabaseCommand_AllAlbums* cmd = new DatabaseCommand_AllAlbums( collection );
    cmd->setLimit( amount );
    cmd->setSortOrder( order );
    cmd->setSortDescending( true );

    connect( cmd, SIGNAL( albums( QList<Tomahawk::album_ptr>, Tomahawk::collection_ptr ) ),
                    SLOT( onAlbumsAdded( QList<Tomahawk::album_ptr>, Tomahawk::collection_ptr ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
AlbumModel::onAlbumsAdded( const QList<Tomahawk::album_ptr>& albums, const Tomahawk::collection_ptr& collection )
{
    if ( !albums.count() )
        return;

    int c = rowCount( QModelIndex() );
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + albums.count() - 1;

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    AlbumItem* albumitem;
    foreach( const album_ptr& album, albums )
    {
        albumitem = new AlbumItem( album, m_rootItem );
        albumitem->cover = m_defaultCover;
        albumitem->index = createIndex( m_rootItem->children.count() - 1, 0, albumitem );

        QString imgurl = "http://ws.audioscrobbler.com/2.0/?method=album.imageredirect&artist=%1&album=%2&size=large&api_key=7a90f6672a04b809ee309af169f34b8b";
        QNetworkRequest req( imgurl.arg( album->artist()->name() ).arg( album->name() ) );
        req.setAttribute( QNetworkRequest::User, (qlonglong)albumitem );
        QNetworkReply* reply = APP->nam()->get( req );
        connect( reply, SIGNAL( finished() ), SLOT( onCoverArtDownloaded() ) );

        connect( albumitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();
    qDebug() << rowCount( QModelIndex() );
}


void
AlbumModel::onCoverArtDownloaded()
{
    QNetworkReply* reply = qobject_cast<QNetworkReply*>( sender() );

    QUrl redir = reply->attribute( QNetworkRequest::RedirectionTargetAttribute ).toUrl();
    if ( redir.isEmpty() )
    {
        const QByteArray ba = reply->readAll();
        if ( ba.length() )
        {
            QPixmap pm;
            pm.loadFromData( ba );

            qlonglong pptr = reply->request().attribute( QNetworkRequest::User ).toLongLong();
            AlbumItem* ai = reinterpret_cast<AlbumItem*>(pptr);

            if ( pm.isNull() || reply->url().toString().startsWith( LASTFM_DEFAULT_COVER ) )
            {
                ai->cover = m_defaultCover;
            }
            else
            {
                ai->setCover( pm.scaled( QSize( 150, 150 ), Qt::IgnoreAspectRatio, Qt::SmoothTransformation ) );
            }
        }
    }
    else
    {
        // Follow HTTP redirect
        QNetworkRequest req( redir );
        req.setAttribute( QNetworkRequest::User, reply->request().attribute( QNetworkRequest::User ) );
        QNetworkReply* reply = APP->nam()->get( req );
        connect( reply, SIGNAL( finished() ), SLOT( onCoverArtDownloaded() ) );
    }

    reply->deleteLater();
}


void
AlbumModel::onDataChanged()
{
    AlbumItem* p = (AlbumItem*)sender();
    emit dataChanged( p->index, p->index.sibling( p->index.row(), columnCount( QModelIndex() ) - 1 ) );
}
