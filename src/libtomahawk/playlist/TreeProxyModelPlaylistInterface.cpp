/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "TreeProxyModelPlaylistInterface.h"

#include "TreeProxyModel.h"

#include "Source.h"
#include "Query.h"
#include "database/Database.h"
#include "database/DatabaseImpl.h"
#include "database/DatabaseCommand_AllAlbums.h"
#include "PlayableItem.h"
#include "utils/Logger.h"

using namespace Tomahawk;


TreeProxyModelPlaylistInterface::TreeProxyModelPlaylistInterface( TreeProxyModel* proxyModel )
    : PlaylistInterface()
    , m_proxyModel( proxyModel )
    , m_repeatMode( PlaylistModes::NoRepeat )
    , m_shuffled( false )
{
}


TreeProxyModelPlaylistInterface::~TreeProxyModelPlaylistInterface()
{
    m_proxyModel.clear();
}


QString
TreeProxyModelPlaylistInterface::filter() const
{
    if ( m_proxyModel.isNull() )
        return 0;
    TreeProxyModel* proxyModel = m_proxyModel.data();
    return proxyModel->filterRegExp().pattern();
}


int
TreeProxyModelPlaylistInterface::trackCount() const
{
    if ( m_proxyModel.isNull() )
        return 0;
    TreeProxyModel* proxyModel = m_proxyModel.data();
    return proxyModel->rowCount( QModelIndex() );
}


void
TreeProxyModelPlaylistInterface::setCurrentIndex( qint64 index )
{
    PlaylistInterface::setCurrentIndex( index );

    PlayableItem* item = static_cast<PlayableItem*>( (void*)index );
    if ( index < 0 || !item )
    {
        m_proxyModel.data()->setCurrentIndex( QModelIndex() );
    }
    else
    {
        m_proxyModel.data()->setCurrentIndex( m_proxyModel.data()->mapFromSource( item->index ) );
    }
}


qint64
TreeProxyModelPlaylistInterface::siblingIndex( int itemsAway, qint64 rootIndex ) const
{
    if ( m_proxyModel.isNull() )
        return -1;

    TreeProxyModel* proxyModel = m_proxyModel.data();
    QModelIndex idx;

    if ( rootIndex == -1 )
    {
        idx = proxyModel->currentIndex();
    }
    else
    {
        PlayableItem* pitem = static_cast<PlayableItem*>( (void*)rootIndex );
        if ( !pitem )
            return -1;

        idx = proxyModel->mapFromSource( pitem->index );
    }
    if ( !idx.isValid() )
        return -1;

    if ( m_shuffled )
    {
        idx = proxyModel->index( qrand() % proxyModel->rowCount( idx.parent() ), 0, idx.parent() );
    }
    else
    {
        if ( m_repeatMode != PlaylistModes::RepeatOne )
            idx = proxyModel->index( idx.row() + ( itemsAway > 0 ? 1 : -1 ), 0, idx.parent() );
    }

    if ( !idx.isValid() && m_repeatMode == PlaylistModes::RepeatAll )
    {
        if ( itemsAway > 0 )
        {
            // reset to first item
            idx = proxyModel->index( 0, 0, proxyModel->currentIndex().parent() );
        }
        else
        {
            // reset to last item
            idx = proxyModel->index( proxyModel->rowCount( proxyModel->currentIndex().parent() ) - 1, 0, proxyModel->currentIndex().parent() );
        }
    }

    // Try to find the next available PlaylistItem (with results)
    while ( idx.isValid() )
    {
        PlayableItem* item = proxyModel->itemFromIndex( proxyModel->mapToSource( idx ) );
        if ( item )
        {
            return (qint64)( item->index.internalPointer() );
        }

        idx = proxyModel->index( idx.row() + ( itemsAway > 0 ? 1 : -1 ), 0, idx.parent() );
    }

    return -1;
}


Tomahawk::result_ptr
TreeProxyModelPlaylistInterface::currentItem() const
{
    if ( m_proxyModel.isNull() )
        return Tomahawk::result_ptr();
    TreeProxyModel* proxyModel = m_proxyModel.data();

    PlayableItem* item = proxyModel->itemFromIndex( proxyModel->mapToSource( proxyModel->currentIndex() ) );
    if ( item && !item->result().isNull() && item->result()->isOnline() )
        return item->result();

    return Tomahawk::result_ptr();
}


QList< Tomahawk::query_ptr >
TreeProxyModelPlaylistInterface::tracks() const
{
    Q_ASSERT( false );
    QList< Tomahawk::query_ptr > queries;
    return queries;
}


qint64
TreeProxyModelPlaylistInterface::indexOfResult( const result_ptr& result ) const
{
    if ( m_proxyModel.isNull() )
        return -1;

    PlayableItem* item = m_proxyModel.data()->itemFromResult( result );
    if ( item )
    {
        return (qint64)( item->index.internalPointer() );
    }

    return -1;
}


qint64
TreeProxyModelPlaylistInterface::indexOfQuery( const query_ptr& query ) const
{
    if ( m_proxyModel.isNull() )
        return -1;

    PlayableItem* item = m_proxyModel.data()->itemFromQuery( query );
    if ( item )
    {
        return (qint64)( item->index.internalPointer() );
    }

    return -1;
}


Tomahawk::query_ptr
TreeProxyModelPlaylistInterface::queryAt( qint64 index ) const
{
    if ( m_proxyModel.isNull() )
        return query_ptr();

    PlayableItem* item = static_cast<PlayableItem*>( (void*)index );
    if ( item && item->query() )
        return item->query();
    if ( item && item->result() )
        return item->result()->toQuery();

    return query_ptr();
}


Tomahawk::result_ptr
TreeProxyModelPlaylistInterface::resultAt( qint64 index ) const
{
    if ( m_proxyModel.isNull() )
        return result_ptr();

    PlayableItem* item = static_cast<PlayableItem*>( (void*)index );
    if ( item && item->result() )
        return item->result();

    return result_ptr();
}
