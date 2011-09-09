/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "treeproxymodel.h"

#include <QListView>

#include "query.h"
#include "database/database.h"
#include "database/databasecommand_allalbums.h"
#include "utils/logger.h"


TreeProxyModel::TreeProxyModel( QObject* parent )
    : QSortFilterProxyModel( parent )
    , PlaylistInterface( this )
    , m_artistsFilterCmd( 0 )
    , m_model( 0 )
    , m_repeatMode( PlaylistInterface::NoRepeat )
    , m_shuffled( false )
{
    setFilterCaseSensitivity( Qt::CaseInsensitive );
    setSortCaseSensitivity( Qt::CaseInsensitive );
    setDynamicSortFilter( true );

    setSourceTreeModel( 0 );
}


void
TreeProxyModel::setSourceModel( QAbstractItemModel* sourceModel )
{
    Q_UNUSED( sourceModel );
    qDebug() << "Explicitly use setSourceTreeModel instead";
    Q_ASSERT( false );
}


void
TreeProxyModel::setSourceTreeModel( TreeModel* sourceModel )
{
    m_model = sourceModel;

    if ( m_model )
    {
        if ( m_model->metaObject()->indexOfSignal( "trackCountChanged(uint)" ) > -1 )
            connect( m_model, SIGNAL( trackCountChanged( unsigned int ) ), SIGNAL( sourceTrackCountChanged( unsigned int ) ) );

        connect( m_model, SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( onRowsInserted( QModelIndex, int, int ) ) );
    }

    QSortFilterProxyModel::setSourceModel( sourceModel );
}


void
TreeProxyModel::onRowsInserted( const QModelIndex& parent, int /* start */, int /* end */ )
{
    if ( m_filter.isEmpty() )
        return;
    if ( sender() != m_model )
        return;

    TreeModelItem* pi = m_model->itemFromIndex( m_model->index( parent.row(), 0, parent.parent() ) );
    if ( pi->artist().isNull() )
        return;

    DatabaseCommand_AllAlbums* cmd = new DatabaseCommand_AllAlbums( m_model->collection() );
    cmd->setArtist( pi->artist() );
    cmd->setFilter( m_filter );

    connect( cmd, SIGNAL( albums( QList<Tomahawk::album_ptr>, QVariant ) ),
                    SLOT( onFilterAlbums( QList<Tomahawk::album_ptr> ) ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
TreeProxyModel::setFilter( const QString& pattern )
{
    emit filteringStarted();

    m_filter = pattern;
    m_albumsFilter.clear();

    if ( m_artistsFilterCmd )
    {
        disconnect( m_artistsFilterCmd, SIGNAL( artists( QList<Tomahawk::artist_ptr> ) ),
                    this,                 SLOT( onFilterArtists( QList<Tomahawk::artist_ptr> ) ) );

        m_artistsFilterCmd = 0;
    }

    if ( m_filter.isEmpty() )
    {
        filterFinished();
    }
    else
    {
        DatabaseCommand_AllArtists* cmd = new DatabaseCommand_AllArtists( m_model->collection() );
        cmd->setFilter( pattern );
        m_artistsFilterCmd = cmd;

        connect( cmd, SIGNAL( artists( QList<Tomahawk::artist_ptr> ) ),
                        SLOT( onFilterArtists( QList<Tomahawk::artist_ptr> ) ) );

        Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
    }
}


void
TreeProxyModel::onFilterArtists( const QList<Tomahawk::artist_ptr>& artists )
{
    bool finished = true;
    m_artistsFilter = artists;

    foreach ( const Tomahawk::artist_ptr& artist, artists )
    {
        QModelIndex idx = m_model->indexFromArtist( artist );
        if ( m_model->rowCount( idx ) )
        {
            finished = false;

            DatabaseCommand_AllAlbums* cmd = new DatabaseCommand_AllAlbums( m_model->collection() );
            cmd->setArtist( artist );
            cmd->setFilter( m_filter );

            connect( cmd, SIGNAL( albums( QList<Tomahawk::album_ptr>, QVariant ) ),
                            SLOT( onFilterAlbums( QList<Tomahawk::album_ptr> ) ) );

            Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
        }
    }

    if ( finished )
        filterFinished();
}


void
TreeProxyModel::onFilterAlbums( const QList<Tomahawk::album_ptr>& albums )
{
    m_albumsFilter << albums;
    filterFinished();
}


void
TreeProxyModel::filterFinished()
{
    m_artistsFilterCmd = 0;
    PlaylistInterface::setFilter( m_filter );
    setFilterRegExp( m_filter );

    emit filterChanged( m_filter );
    emit trackCountChanged( trackCount() );
    emit filteringFinished();
}


bool
TreeProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex& sourceParent ) const
{
    TreeModelItem* pi = sourceModel()->itemFromIndex( sourceModel()->index( sourceRow, 0, sourceParent ) );
    Q_ASSERT( pi );

    if ( m_model->mode() == TreeModel::Database && !pi->result().isNull() )
    {
        QList< Tomahawk::result_ptr > rl = m_cache.values( sourceParent );
        foreach ( const Tomahawk::result_ptr& result, rl )
        {
            if ( result->track() == pi->result()->track() &&
               ( result->albumpos() == pi->result()->albumpos() || result->albumpos() == 0 ) )
            {
                if ( result.data() != pi->result().data() )
                    return false;
            }
        }

        for ( int i = 0; i < sourceModel()->rowCount( sourceParent ); i++ )
        {
            if ( i == sourceRow )
                continue;

            TreeModelItem* ti = sourceModel()->itemFromIndex( sourceModel()->index( i, 0, sourceParent ) );

            if ( ti->name() == pi->name() &&
               ( ti->result()->albumpos() == pi->result()->albumpos() || ti->result()->albumpos() == 0 ) )
            {
                if ( !pi->result()->isOnline() && ti->result()->isOnline() )
                    return false;

                if ( ti->result()->collection()->source()->isLocal() )
                    return false;
            }
        }

//        tDebug() << "Accepting:" << pi->result()->toString() << pi->result()->collection()->source()->id();
        m_cache.insertMulti( sourceParent, pi->result() );
    }

    if ( m_filter.isEmpty() )
        return true;

    if ( !pi->artist().isNull() )
        return m_artistsFilter.contains( pi->artist() );
    if ( !pi->album().isNull() )
        return m_albumsFilter.contains( pi->album() );

    QStringList sl = m_filter.split( " ", QString::SkipEmptyParts );
    foreach( const QString& s, sl )
    {
        if ( !pi->name().contains( s, Qt::CaseInsensitive ) &&
             !pi->albumName().contains( s, Qt::CaseInsensitive ) &&
             !pi->artistName().contains( s, Qt::CaseInsensitive ) )
        {
            return false;
        }
    }

    return true;
}


bool
TreeProxyModel::lessThan( const QModelIndex& left, const QModelIndex& right ) const
{
    TreeModelItem* p1 = sourceModel()->itemFromIndex( left );
    TreeModelItem* p2 = sourceModel()->itemFromIndex( right );

    if ( !p1 )
        return true;
    if ( !p2 )
        return false;

    if ( !p1->result().isNull() && p2->result().isNull() )
        return true;
    if ( p1->result().isNull() && !p2->result().isNull() )
        return false;

    const QString& lefts = textForItem( p1 );
    const QString& rights = textForItem( p2 );

    if ( !p1->result().isNull() )
    {
        if ( p1->result()->albumpos() != p2->result()->albumpos() )
            return p1->result()->albumpos() < p2->result()->albumpos();

        if ( lefts == rights )
            return (qint64)&p1 < (qint64)&p2;
    }

    return QString::localeAwareCompare( lefts, rights ) < 0;
}


void
TreeProxyModel::removeIndex( const QModelIndex& index )
{
    qDebug() << Q_FUNC_INFO;

    if ( !sourceModel() )
        return;
    if ( index.column() > 0 )
        return;

    sourceModel()->removeIndex( mapToSource( index ) );
}


void
TreeProxyModel::removeIndexes( const QList<QModelIndex>& indexes )
{
    if ( !sourceModel() )
        return;

    foreach( const QModelIndex& idx, indexes )
    {
        removeIndex( idx );
    }
}


bool
TreeProxyModel::hasNextItem()
{
    return !( siblingItem( 1, true ).isNull() );
}


Tomahawk::result_ptr
TreeProxyModel::siblingItem( int itemsAway )
{
    return siblingItem( itemsAway, false );
}


Tomahawk::result_ptr
TreeProxyModel::siblingItem( int itemsAway, bool readOnly )
{
    qDebug() << Q_FUNC_INFO;

    QModelIndex idx = currentIndex();

    // Try to find the next available PlaylistItem (with results)
    if ( idx.isValid() ) do
    {
        idx = index( idx.row() + ( itemsAway > 0 ? 1 : -1 ), 0, idx.parent() );
        if ( !idx.isValid() )
            break;

        TreeModelItem* item = itemFromIndex( mapToSource( idx ) );
        if ( item && item->result()->isOnline() )
        {
            qDebug() << "Next PlaylistItem found:" << item->result()->url();
            if ( !readOnly )
                setCurrentIndex( idx );
            return item->result();
        }
    }
    while ( idx.isValid() );

    if ( !readOnly )
        setCurrentIndex( QModelIndex() );
    return Tomahawk::result_ptr();
}


Tomahawk::result_ptr
TreeProxyModel::currentItem() const
{
    TreeModelItem* item = itemFromIndex( mapToSource( currentIndex() ) );
    if ( item && !item->result().isNull() && item->result()->isOnline() )
        return item->result();
    return Tomahawk::result_ptr();
}


QString
TreeProxyModel::textForItem( TreeModelItem* item ) const
{
    if ( !item )
        return QString();

    if ( !item->artist().isNull() )
    {
        return item->artist()->sortname();
    }
    else if ( !item->album().isNull() )
    {
        return DatabaseImpl::sortname( item->album()->name() );
    }
    else if ( !item->result().isNull() )
    {
        return DatabaseImpl::sortname( item->result()->track() );
    }
    else if ( !item->query().isNull() )
    {
        return item->query()->track();
    }

    return QString();
}
