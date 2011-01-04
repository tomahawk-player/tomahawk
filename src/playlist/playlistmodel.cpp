#include "playlistmodel.h"

#include <QDebug>
#include <QMimeData>
#include <QTreeView>

#include "album.h"

#include "database/database.h"
#include "database/databasecommand_playbackhistory.h"

using namespace Tomahawk;


PlaylistModel::PlaylistModel( QObject* parent )
    : TrackModel( parent )
    , m_waitForUpdate( false )
{
    qDebug() << Q_FUNC_INFO;

    setReadOnly( false );
}


PlaylistModel::~PlaylistModel()
{
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
    if ( !m_playlist.isNull() )
        disconnect( m_playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), this, SLOT( onRevisionLoaded( Tomahawk::PlaylistRevision ) ) );

    if ( rowCount( QModelIndex() ) )
    {
        emit beginRemoveRows( QModelIndex(), 0, rowCount( QModelIndex() ) - 1 );
        delete m_rootItem;
        emit endRemoveRows();
        m_rootItem = new PlItem( 0, this );
    }

    m_playlist = playlist;
    connect( playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), SLOT( onRevisionLoaded( Tomahawk::PlaylistRevision ) ) );

    setReadOnly( !m_playlist->author()->isLocal() );

    PlItem* plitem;
    QList<plentry_ptr> entries = playlist->entries();
    if ( entries.count() )
    {
        int c = rowCount( QModelIndex() );

        qDebug() << "starting loading" << playlist->title();
        emit loadingStarts();
        emit beginInsertRows( QModelIndex(), c, c + entries.count() - 1 );

        foreach( const plentry_ptr& entry, entries )
        {
            qDebug() << entry->query()->toString();
            plitem = new PlItem( entry, m_rootItem );
            plitem->index = createIndex( m_rootItem->children.count() - 1, 0, plitem );

            connect( plitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
        }

        emit endInsertRows();
    }

    qDebug() << rowCount( QModelIndex() );
    emit loadingFinished();
}


void
PlaylistModel::loadAlbum( const Tomahawk::album_ptr& album )
{
    if ( album.isNull() )
        return;

    if ( rowCount( QModelIndex() ) )
    {
        emit beginRemoveRows( QModelIndex(), 0, rowCount( QModelIndex() ) - 1 );
        delete m_rootItem;
        emit endRemoveRows();
        m_rootItem = new PlItem( 0, this );
    }

    m_playlist.clear();
    setReadOnly( false );

    connect( album.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ),
                             SLOT( onTracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::collection_ptr ) ) );

    onTracksAdded( album->tracks(), album->collection() );
}


void
PlaylistModel::loadHistory( const Tomahawk::source_ptr& source, unsigned int amount )
{
    if ( rowCount( QModelIndex() ) )
    {
        emit beginRemoveRows( QModelIndex(), 0, rowCount( QModelIndex() ) - 1 );
        delete m_rootItem;
        emit endRemoveRows();
        m_rootItem = new PlItem( 0, this );
    }

    m_playlist.clear();
    setReadOnly( true );

    DatabaseCommand_PlaybackHistory* cmd = new DatabaseCommand_PlaybackHistory( source );

    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ),
                    SLOT( onTracksAdded( QList<Tomahawk::query_ptr> ) ), Qt::QueuedConnection );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
PlaylistModel::appendTrack( const Tomahawk::query_ptr& query )
{
    if ( query.isNull() )
        return;

    QList< Tomahawk::query_ptr > ql;
    ql << query;

    onTracksAdded( ql );
}


void
PlaylistModel::onTracksAdded( const QList<Tomahawk::query_ptr>& tracks, const Tomahawk::collection_ptr& collection )
{
    if ( !tracks.count() )
        return;

    int c = rowCount( QModelIndex() );
    QPair< int, int > crows;
    crows.first = c;
    crows.second = c + tracks.count() - 1;

    emit beginInsertRows( QModelIndex(), crows.first, crows.second );

    PlItem* plitem;
    foreach( const query_ptr& query, tracks )
    {
        plentry_ptr entry = plentry_ptr( new PlaylistEntry() );
        entry->setQuery( query );

        plitem = new PlItem( entry, m_rootItem );
        plitem->index = createIndex( m_rootItem->children.count() - 1, 0, plitem );

        connect( plitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
    }

    emit endInsertRows();
    emit trackCountChanged( rowCount( QModelIndex() ) );
    qDebug() << rowCount( QModelIndex() );
}


void
PlaylistModel::onDataChanged()
{
    PlItem* p = (PlItem*)sender();
    if ( p && p->index.isValid() )
        emit dataChanged( p->index, p->index.sibling( p->index.row(), columnCount() - 1 ) );
}


void
PlaylistModel::onRevisionLoaded( Tomahawk::PlaylistRevision revision )
{
    qDebug() << Q_FUNC_INFO;

    if ( m_waitForUpdate )
    {
        qDebug() << m_playlist->currentrevision() << revision.revisionguid;
        m_waitForUpdate = false;
    }
    else
        loadPlaylist( m_playlist );
}


bool
PlaylistModel::dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    if ( action == Qt::IgnoreAction || isReadOnly() )
        return true;

    if ( !data->hasFormat( "application/tomahawk.query.list" ) && !data->hasFormat( "application/tomahawk.plentry.list" ) )
        return false;

    int beginRow;
    if ( row != -1 )
        beginRow = row;
    else if ( parent.isValid() )
        beginRow = parent.row();
    else
        beginRow = rowCount( QModelIndex() );

    qDebug() << data->formats();

    if ( data->hasFormat( "application/tomahawk.query.list" ) )
    {
        QByteArray itemData = data->data( "application/tomahawk.query.list" );
        QDataStream stream( &itemData, QIODevice::ReadOnly );
        QList<Tomahawk::query_ptr> queries;

        while ( !stream.atEnd() )
        {
            qlonglong qptr;
            stream >> qptr;

            Tomahawk::query_ptr* query = reinterpret_cast<Tomahawk::query_ptr*>(qptr);
            if ( query && !query->isNull() )
            {
                qDebug() << "Dropped query item:" << query->data()->artist() << "-" << query->data()->track() << action;
                queries << *query;
            }
        }

        emit beginInsertRows( QModelIndex(), beginRow, beginRow + queries.count() - 1 );
        foreach( const Tomahawk::query_ptr& query, queries )
        {
            plentry_ptr e( new PlaylistEntry() );
            e->setGuid( uuid() );

            if ( query->results().count() )
                e->setDuration( query->results().at( 0 )->duration() );
            else
                e->setDuration( 0 );

            e->setLastmodified( 0 );
            e->setAnnotation( "" ); // FIXME
            e->setQuery( query );

            PlItem* plitem = new PlItem( e, m_rootItem, beginRow );
            plitem->index = createIndex( beginRow++, 0, plitem );

            connect( plitem, SIGNAL( dataChanged() ), SLOT( onDataChanged() ) );
        }
        emit endInsertRows();

        if ( action == Qt::CopyAction )
        {
            onPlaylistChanged();
        }
    }

    return true;
}


void
PlaylistModel::onPlaylistChanged( bool waitForUpdate )
{
    qDebug() << Q_FUNC_INFO;

    if ( m_playlist.isNull() )
        return;

    QList<plentry_ptr> l = playlistEntries();

    foreach( const plentry_ptr& ple, l )
    {
        qDebug() << "updateinternal:" << ple->query()->toString();
    }

    m_waitForUpdate = waitForUpdate;
    QString newrev = uuid();
    m_playlist->createNewRevision( newrev, m_playlist->currentrevision(), l );
}


QList<Tomahawk::plentry_ptr>
PlaylistModel::playlistEntries() const
{
    QList<plentry_ptr> l;
    for ( int i = 0; i < rowCount( QModelIndex() ); i++ )
    {
        QModelIndex idx = index( i, 0, QModelIndex() );
        if ( !idx.isValid() )
            continue;

        PlItem* item = itemFromIndex( idx );
        if ( item )
            l << item->entry();
    }

    return l;
}


void
PlaylistModel::removeIndex( const QModelIndex& index, bool moreToCome )
{
    if ( isReadOnly() )
        return;

    TrackModel::removeIndex( index );

    if ( !moreToCome && !m_playlist.isNull() )
    {
        onPlaylistChanged();
    }
}
