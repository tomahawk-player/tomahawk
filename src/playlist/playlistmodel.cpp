#include "playlistmodel.h"

#include <QDebug>
#include <QMimeData>
#include <QSortFilterProxyModel>

#include "playlistitem.h"
#include "playlistview.h"
#include "playlistmodelworker.h"
#include "playlistproxymodel.h"
#include "animatedrowremover.h"
#include "databasecollection.h"
#include "tomahawk/collection.h"
#include "tomahawk/tomahawkapp.h"

using namespace Tomahawk;

PlaylistModel::PlaylistModel( QObject* parent )
    : QStandardItemModel( parent )
    , m_collectionCount( 0 )
    , m_sourceCount( 0 )
    , m_remoteUpdate( false )
    , m_readOnly( false )
    , m_nowPlayingIcon( QIcon( RESPATH "images/now-playing-speaker.png" ) )
    , m_sources( 0 )
    , m_busy( false )
{
    qDebug() << Q_FUNC_INFO;

    setupHeaders();
    setColumnCount( 6 );
    setReadOnly( true );

    connect( this, SIGNAL( rowsAboutToBeRemoved( QModelIndex, int, int ) ), SLOT( onRowsAboutToBeRemoved( QModelIndex, int, int ) ) );
    connect( this, SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( onRowsInserted( QModelIndex, int, int ) ) );
    connect( this, SIGNAL( rowsRemoved( QModelIndex, int, int ) ), SLOT( onRowsRemoved( QModelIndex, int, int ) ) );
}


void
PlaylistModel::setupHeaders()
{
    QStringList headers;
    headers << tr( "Artist" ) << tr( "Track" ) << tr( "Album" ) << tr( "Duration" ) << tr( "Bitrate" ) << tr( "Origin" );

    setHorizontalHeaderLabels( headers );
}


void
PlaylistModel::updateStats()
{
    emit numTracksChanged( rowCount() );
    emit numArtistsChanged( m_artists.size() );
    emit numSourcesChanged( m_sources );
    emit collectionCountChanged( m_collectionCount );
}


bool
PlaylistModel::appendItem( Tomahawk::query_ptr query, bool emitsig )
{
    PlaylistItem* item = new PlaylistItem( query );
    invisibleRootItem()->appendRow( item->columns() );

    if( emitsig )
        emit numTracksChanged( rowCount() );

    return true;
}


bool
PlaylistModel::appendItems( const QList<PlaylistItem*>& items, bool emitsig )
{
//    qDebug() << Q_FUNC_INFO << items.length();
//    ProgressTreeView* tree = ((ProgressTreeView*)( parent()->parent() ));
//    tree->setProgressStarted( items.length() );

//    PlaylistProxyModel* p = ((PlaylistProxyModel*)( parent() ));
    blockSignals( true );

    foreach( PlaylistItem* i, items )
    {
        invisibleRootItem()->appendRow( i->columns() );
    }

    if ( emitsig )
        emit numTracksChanged( rowCount() );

    blockSignals( false );

//    tree->setProgressEnded();

    return true;
}


bool
PlaylistModel::insertItems( int pos, const QList<PlaylistItem*>& items, bool emitsig )
{
    qDebug() << Q_FUNC_INFO << items.length();
//    PlaylistProxyModel* p = ((PlaylistProxyModel*)( parent() ));

    blockSignals( true );

    foreach( PlaylistItem* i, items )
    {
        invisibleRootItem()->insertRow( pos++, i->columns() );
    }

    if ( emitsig )
        emit numTracksChanged( rowCount() );

    blockSignals( false );

    return true;
}


void
PlaylistModel::removeItems( const QList<PlaylistItem*>& items, bool emitsig, bool animated )
{
    if ( emitsig )
    {
        emit numArtistsChanged( m_artists.size() );
        emit numTracksChanged( rowCount() - items.count() );
    }

    // Now trigger the row removal
    AnimatedRowRemover* arr = new AnimatedRowRemover( ((PlaylistView*)parent()->parent()),
                                                      this,
                                                      items ); // will auto delete

    connect( arr, SIGNAL( finished() ), SLOT( onRowRemoverFinished() ) );
    arr->start( animated );
}


void
PlaylistModel::loadPlaylist( const playlist_ptr& playlist )
{
    m_playlist = playlist;
    if ( m_playlist->author()->isLocal() )
        setReadOnly( false );
    else
        setReadOnly( true );

    connect( m_playlist.data(), SIGNAL( revisionLoaded( Tomahawk::PlaylistRevision ) ), SLOT( onRevisionLoaded( Tomahawk::PlaylistRevision ) ) );
    appendPlaylistEntries( playlist->entries(), collection_ptr( 0 ) ); // FIXME: collection_ptr 0? really?
}


void
PlaylistModel::addCollection( const collection_ptr& collection )
{
    qDebug() << Q_FUNC_INFO << collection->name()
                            << collection->source()->id()
                            << collection->source()->userName();

    m_busy = true;

    collection->loadTracks( this, "onTracksAdded" );
    emit collectionCountChanged( m_collectionCount++ ); //FIXME pre-increment for clarity?

    connect( collection.data(), SIGNAL( tracksAdded( const QList<QVariant>&, Tomahawk::collection_ptr ) ),
             SLOT( onTracksAdded( const QList<QVariant>&, Tomahawk::collection_ptr ) ) );

    connect( collection.data(), SIGNAL( tracksRemoved( const QList<QVariant>&, Tomahawk::collection_ptr ) ),
             SLOT( onTracksRemoved( const QList<QVariant>&, Tomahawk::collection_ptr ) ) );
}


void
PlaylistModel::removeCollection( const collection_ptr& collection )
{
    emit collectionCountChanged( m_collectionCount-- );
    m_busy = true;

    QList<PlaylistItem*> list;
    int rows = rowCount();
    for ( int i = rows - 1; i >= 0; i-- )
    {
        PlaylistItem* item = indexToPlaylistItem( index( i, 0 ) );
        if ( item && item->query()->numResults() )
        {
            foreach( const result_ptr& result, item->query()->results() )
            {
                if ( result->collection() == collection )
                {
                    // must be the last collection that was just
                    // deactivated, so remove the item from the model
                    if ( item->query()->numResults() == 1 )
                    {
                        if ( m_currentItem == QPersistentModelIndex( item->index() ) )
                            setCurrentItem( QModelIndex() );

                        item->setBeingRemoved( true );
                        list << item;
                        unsigned int g = m_artists.value( result->artist(), 0 );
                        if ( g == 1 )
                            m_artists.remove( result->artist() );
                        else if ( g > 0 )
                            m_artists[result->artist()]--;
                    }
                }
            }
        }
    }

    removeItems( list, true, true );

    disconnect( collection.data(), SIGNAL( tracksAdded( const QList<QVariant>&, Tomahawk::collection_ptr ) ),
                this, SLOT( onTracksAdded( const QList<QVariant>&, Tomahawk::collection_ptr ) ) );

    disconnect( collection.data(), SIGNAL( tracksRemoved( const QList<QVariant>&, Tomahawk::collection_ptr ) ),
                this, SLOT( onTracksRemoved( const QList<QVariant>&, Tomahawk::collection_ptr ) ) );
}


void
PlaylistModel::addSource( const source_ptr& source )
{
    m_sources++;
    emit numSourcesChanged( m_sources );
    addCollection( source->collection() );
}


void
PlaylistModel::removeSource( const source_ptr& source )
{
    m_sources--;
    emit numSourcesChanged( m_sources );
    removeCollection( source->collection() );
}


void
PlaylistModel::onTracksAdded( const QList<QVariant>& tracks, collection_ptr collection )
{
    qDebug() << Q_FUNC_INFO;

    QList<Tomahawk::query_ptr> queries;
    foreach( const QVariant& v, tracks )
    {
        Tomahawk::query_ptr query = query_ptr( new Query( v ) );
        queries << query;
    }

    appendTracks( queries, collection );
}


void
PlaylistModel::appendTracks( const QList<Tomahawk::query_ptr>& queries, collection_ptr collection )
{
    qDebug() << "Adding queries:" << queries.length();
    m_busy = true;

    PlaylistModelWorker* worker = new PlaylistModelWorker( queries, this, collection );
    connect( worker, SIGNAL( appendBatch( const QList<PlaylistItem*>& , bool ) ),
                       SLOT( appendItems( const QList<PlaylistItem*>& , bool ) ), Qt::QueuedConnection );
    connect( worker, SIGNAL( finished() ), SLOT( workerFinished() ) );

    emit numTracksChanged( rowCount() + worker->queries().length() );

    qDebug() << Q_FUNC_INFO << "Starting worker. our thread" << thread();
    m_remoteUpdate = true;
    worker->start();
}


void
PlaylistModel::appendPlaylistEntries( const QList<Tomahawk::plentry_ptr>& entries, Tomahawk::collection_ptr collection )
{
    qDebug() << "Adding entries:" << entries.length();
    m_busy = true;

    PlaylistModelWorker* worker = new PlaylistModelWorker( entries, this, collection );
    connect( worker, SIGNAL( appendBatch( const QList<PlaylistItem*>& , bool ) ),
                       SLOT( appendItems( const QList<PlaylistItem*>& , bool ) ), Qt::QueuedConnection );
    connect( worker, SIGNAL( finished() ), SLOT( workerFinished() ), Qt::QueuedConnection );

    emit numTracksChanged( rowCount() + worker->entries().length() );

    qDebug() << Q_FUNC_INFO << "Starting worker. our thread" << thread();
    m_remoteUpdate = true;
    worker->start();
}


void PlaylistModel::workerFinished()
{
    qDebug() << "Worker finished";

    updateStats();
    m_remoteUpdate = false;
    m_busy = false;

    emit layoutChanged();
}


void
PlaylistModel::onTracksRemoved( const QList<QVariant>& tracks, collection_ptr collection )
{
    // FIXME
}


QList<Tomahawk::plentry_ptr> const
PlaylistModel::playlistEntries()
{
    QList<plentry_ptr> list;

    for ( int i = 0; i < rowCount(); i++ )
    {
        QModelIndex idx = index( i, 0 );
        if ( !idx.isValid() )
            continue;

        PlaylistItem* item = indexToPlaylistItem( idx );
        if ( item )
            list << item->entry();
    }

    return list;
}


void
PlaylistModel::setCurrentItem( const QModelIndex& index )
{
    PlaylistItem* item = indexToPlaylistItem( m_currentItem );
    if ( item && item->columns().count() )
    {
        item->columns().at( 0 )->setIcon( QIcon( "" ) );
    }

    m_currentItem = index;

    item = indexToPlaylistItem( m_currentItem );
    if ( item && item->columns().count() )
    {
        item->columns().at( 0 )->setIcon( m_nowPlayingIcon );
    }
}


PlaylistItem*
PlaylistModel::previousItem()
{
    return siblingItem( -1 );
}


PlaylistItem*
PlaylistModel::nextItem()
{
    return siblingItem( 1 );
}


PlaylistItem*
PlaylistModel::siblingItem( int itemsAway )
{
    QModelIndex idx;

    if ( m_currentItem.isValid() )
    {
        idx = m_currentItem;
        idx = index( idx.row() + itemsAway, 0, idx.parent() );
    }
    else
        idx = index( 0, 0, idx );

    // Try to find the next available PlaylistItem (with results)
    do
    {
        if ( !idx.isValid() )
            break;


        PlaylistItem* item = indexToPlaylistItem( idx );
        if ( item && item->query()->numResults() )
        {
            qDebug() << "Next PlaylistItem found: " << item->query()->toString() << item->query()->results().at( 0 )->url();
            setCurrentItem( item->index() );
            return item;
        }

        idx = index( idx.row() + ( itemsAway > 0 ? 1 : -1 ), 0, idx.parent() );
    }
    while ( idx.isValid() );

    setCurrentItem( QModelIndex() );
    return 0;
}


unsigned int
PlaylistModel::trackCount()
{
    return rowCount();
}


Qt::DropActions
PlaylistModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}


Qt::ItemFlags
PlaylistModel::flags( const QModelIndex& index ) const
{
    Qt::ItemFlags defaultFlags = QStandardItemModel::flags( index );

    if ( index.isValid() )
        return Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | defaultFlags;
    else
        return Qt::ItemIsDropEnabled | defaultFlags;
}


QStringList
PlaylistModel::mimeTypes() const
{
    QStringList types;
    types << "application/tomahawk.query.list";
    types << "application/tomahawk.plentry.list";
    return types;
}


QMimeData*
PlaylistModel::mimeData( const QModelIndexList &indexes ) const
{
    qDebug() << Q_FUNC_INFO;

    m_currentPlEntry = plentry_ptr();
    QByteArray queryData;
    QByteArray itemData;
    QDataStream queryStream( &queryData, QIODevice::WriteOnly );
    QDataStream itemStream( &itemData, QIODevice::WriteOnly );

    foreach ( const QModelIndex& i, indexes )
    {
        if ( i.column() > 0 )
            continue;

        QModelIndex idx = index( i.row(), 0, i.parent() );
        PlaylistItem* item = indexToPlaylistItem( idx );
        if ( item )
        {
            if ( isPlaylistBacked() )
            {
                const plentry_ptr& entry = item->entry();
                itemStream << qlonglong( &entry );

                if ( m_currentItem == idx )
                    m_currentPlEntry = entry;
            }

            const query_ptr& query = item->query();
            queryStream << qlonglong( &query );
        }
    }

    QMimeData* mimeData = new QMimeData();
    mimeData->setData( "application/tomahawk.query.list", queryData );
    if ( isPlaylistBacked() )
        mimeData->setData( "application/tomahawk.plentry.list", itemData );

    return mimeData;
}


bool
PlaylistModel::dropMimeData( const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent )
{
    if ( action == Qt::IgnoreAction || m_readOnly )
        return true;

    if ( !data->hasFormat( "application/tomahawk.query.list" ) && !data->hasFormat( "application/tomahawk.plentry.list" ) )
        return false;

    int beginRow;
    if ( row != -1 )
        beginRow = row;
    else if ( parent.isValid() )
        beginRow = parent.row();
    else
        beginRow = rowCount();

    m_remoteUpdate = true;
    if ( data->hasFormat( "application/tomahawk.plentry.list" ) )
    {
        QByteArray itemData = data->data( "application/tomahawk.plentry.list" );
        QDataStream stream( &itemData, QIODevice::ReadOnly );
        QList<Tomahawk::plentry_ptr> entries;

        while ( !stream.atEnd() )
        {
            qlonglong qptr;
            stream >> qptr;

            Tomahawk::plentry_ptr* entry = reinterpret_cast<Tomahawk::plentry_ptr*>(qptr);
            if ( entry && !entry->isNull() )
            {
                qDebug() << "Dropped playlist item:" << entry->data()->query()->artist() << "-" << entry->data()->query()->track();
                entries << *entry;
            }
        }

        foreach( const Tomahawk::plentry_ptr& entry, entries )
        {
            PlaylistItem* item = new PlaylistItem( entry );
            invisibleRootItem()->insertRow( beginRow++, item->columns() );

            if ( !m_currentPlEntry.isNull() && m_currentPlEntry->guid() == entry->guid() )
            {
                setCurrentItem( item->index() );
                m_currentPlEntry = plentry_ptr();
            }
        }
    }
    else
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
                qDebug() << "Dropped query item:" << query->data()->artist() << "-" << query->data()->track();
                queries << *query;
            }
        }

        foreach( const Tomahawk::query_ptr& query, queries )
        {
            PlaylistItem* item = new PlaylistItem( query );
            invisibleRootItem()->insertRow( beginRow++, item->columns() );
        }
    }
    m_remoteUpdate = false;

    return true;
}


void
PlaylistModel::updateInternalPlaylist()
{
    if ( !isPlaylistBacked() )
        return;

    QList<Tomahawk::plentry_ptr> e = playlistEntries();
    foreach( const plentry_ptr& ple, e )
    {
        qDebug() << "updateinternal:" << ple->query()->toString();
    }

    m_remoteUpdate = true;
    setReadOnly( true );
    QString newrev = uuid();
    m_playlist->createNewRevision( newrev, m_playlist->currentrevision(), e );
}


void
PlaylistModel::onRevisionLoaded( Tomahawk::PlaylistRevision revision )
{
    if ( !m_remoteUpdate )
    {
        m_remoteUpdate = true;

        foreach( const plentry_ptr& p, revision.newlist )
        {
            qDebug() << "newlist:" << p->query()->toString();
        }

        foreach( const plentry_ptr& p, revision.added )
        {
            qDebug() << "added:" << p->query()->toString();
        }

        foreach( const plentry_ptr& p, revision.removed )
        {
            qDebug() << "removed:" << p->query()->toString();
        }

        qDebug() << rowCount() << revision.newlist.count();
        for ( int i = 0; i < revision.newlist.count(); i++ )
        {
            if ( ( rowCount() - 1 ) < i )
            {
                qDebug() << "item has been added";

                QList<PlaylistItem*> items;
                items << new PlaylistItem( revision.newlist.at( i ) );
                APP->pipeline()->add( items.at( 0 )->query() );

                appendItems( items, collection_ptr( 0 ) );
            }
            else
            {
                PlaylistItem* item = indexToPlaylistItem( index( i, 0 ) );
                qDebug() << "items:" << i << item->query()->toString() << revision.newlist.at( i )->query()->toString();

                if ( item->entry()->guid() != revision.newlist.at( i )->guid() )
                {
                    if ( revision.added.contains( revision.newlist.at( i ) ) )
                    {
                        qDebug() << "item has been added";

                        QList<PlaylistItem*> items;
                        items << new PlaylistItem( revision.newlist.at( i ) );

                        insertItems( i, items, collection_ptr( 0 ) );
                    }
                    // FIXME: add additional check for removed items, iterating over
                    // FIXME: revision.removed only, instead of the entire list as we do below
                    else
                    {
                        qDebug() << "item seems to have moved in the list";
                        QString guid = item->entry()->guid();
                        QList<QStandardItem*> li = takeRow( i );

                        int x = 0;
                        foreach( const plentry_ptr& nlp, revision.newlist )
                        {
                            if ( guid == nlp->guid() )
                                break;
                            x++;
                        }

                        if ( x < revision.newlist.count() )
                            invisibleRootItem()->insertRow( x, li );
                        else
                            delete item;

                        i--;
                    }
                }
            }
        }
    }
    m_remoteUpdate = false;
    emit layoutChanged();

    if ( isPlaylistBacked() && m_playlist->author()->isLocal() )
            setReadOnly( false );
}


void
PlaylistModel::onRowsAboutToBeRemoved( const QModelIndex& parent, int start, int end )
{
//    qDebug() << Q_FUNC_INFO << start << end;

    if ( m_remoteUpdate )
        return;

    for ( int i = start; i <= end; i++ )
    {
        PlaylistItem* item = indexToPlaylistItem( index( i, 0, parent ) );
        if ( item )
            delete item;
    }
}


void
PlaylistModel::onRowsInserted( const QModelIndex& parent, int start, int end )
{
//    qDebug() << Q_FUNC_INFO << start << end;

    if ( m_remoteUpdate )
        return;

    if ( isPlaylistBacked() )
        updateInternalPlaylist();
}


void
PlaylistModel::onRowsRemoved( const QModelIndex& parent, int start, int end )
{
//    qDebug() << Q_FUNC_INFO << start << end;

    if ( m_remoteUpdate )
        return;

    if ( isPlaylistBacked() )
        updateInternalPlaylist();
}


int
PlaylistModel::indexType( const QModelIndex& index )
{
    if ( !index.isValid() )
        return -1;

    QModelIndex idx = index.model()->index( index.row(), 0, index.parent() );
    return idx.data( Qt::UserRole + 1 ).toInt();
}


PlaylistItem*
PlaylistModel::indexToPlaylistItem( const QModelIndex& index )
{
    if ( !index.isValid() )
        return 0;

    QModelIndex idx = index.model()->index( index.row(), 0, index.parent() );
    int type = idx.data( Qt::UserRole + 1 ).toInt();
    if ( type == 0 )
    {
        qlonglong pptr = idx.data( Qt::UserRole ).toLongLong();
        PlaylistItem* item = reinterpret_cast<PlaylistItem*>(pptr);
        if ( item )
            return item;
    }

    return 0;
}
