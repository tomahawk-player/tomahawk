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

#include "PlayableProxyModelPlaylistInterface.h"

#include "PlayableProxyModel.h"
#include "Artist.h"
#include "Album.h"
#include "Query.h"
#include "PlayableItem.h"
#include "Source.h"
#include "utils/Logger.h"

using namespace Tomahawk;


PlayableProxyModelPlaylistInterface::PlayableProxyModelPlaylistInterface( PlayableProxyModel* proxyModel )
    : PlaylistInterface()
    , m_proxyModel( proxyModel )
    , m_repeatMode( PlaylistModes::NoRepeat )
    , m_shuffled( false )
{
    connect( proxyModel, SIGNAL( indexPlayable( QModelIndex ) ), SLOT( onItemsChanged() ) );
    connect( proxyModel, SIGNAL( filterChanged( QString ) ), SLOT( onItemsChanged() ) );
    connect( proxyModel, SIGNAL( itemCountChanged( unsigned int ) ), SLOT( onItemsChanged() ) );
    connect( proxyModel, SIGNAL( currentIndexChanged() ), SLOT( onCurrentIndexChanged() ) );
}


PlayableProxyModelPlaylistInterface::~PlayableProxyModelPlaylistInterface()
{
    tDebug() << Q_FUNC_INFO;
    m_proxyModel.clear();
}


int
PlayableProxyModelPlaylistInterface::trackCount() const
{
    return ( m_proxyModel.isNull() ? 0 : m_proxyModel.data()->rowCount( QModelIndex() ) );
}


QString
PlayableProxyModelPlaylistInterface::filter() const
{
    return ( m_proxyModel.isNull() ? QString() : m_proxyModel.data()->filterRegExp().pattern() );
}


QList< Tomahawk::query_ptr >
PlayableProxyModelPlaylistInterface::tracks() const
{
    if ( m_proxyModel.isNull() )
        return QList< query_ptr >();

    PlayableProxyModel* proxyModel = m_proxyModel.data();
    QList< query_ptr > queries;

    for ( int i = 0; i < proxyModel->rowCount( QModelIndex() ); i++ )
    {
        PlayableItem* item = proxyModel->itemFromIndex( proxyModel->mapToSource( proxyModel->index( i, 0 ) ) );
        if ( item )
            queries << item->query();
    }

    return queries;
}


void
PlayableProxyModelPlaylistInterface::onCurrentIndexChanged()
{
    if ( m_proxyModel.data()->currentIndex().isValid() )
        setCurrentIndex( (qint64) m_proxyModel.data()->mapToSource( m_proxyModel.data()->currentIndex() ).internalPointer() );
    else
        setCurrentIndex( -1 );
}


void
PlayableProxyModelPlaylistInterface::setCurrentIndex( qint64 index )
{
    Q_ASSERT( m_proxyModel );
    if ( m_proxyModel.isNull() )
        return;

    if ( m_currentIndex == index )
        return;
    m_currentIndex = index; // we need to manually set m_currentIndex (protected member from PlaylistInterface) here
                            // because calling m_proxyModel.data()->setCurrentIndex( ... ) will end up emitting a
                            // signal which leads right back here and would cause an infinite loop.

    PlayableItem* item = reinterpret_cast<PlayableItem*>( (void*)index );
    if ( index >= 0 && item )
    {
        if ( m_shuffled && m_shuffleHistory.count() > 1 )
        {
            if ( m_proxyModel.data()->itemFromQuery( m_shuffleHistory.at( m_shuffleHistory.count() - 2 ) ) &&
               ( m_proxyModel.data()->mapFromSource( item->index ) == m_proxyModel.data()->mapFromSource( m_proxyModel.data()->itemFromQuery( m_shuffleHistory.at( m_shuffleHistory.count() - 2 ) )->index ) ) )
            {
                // Note: the following lines aren't by mistake:
                // We detected that we're going to the previous track in our shuffle history and hence we want to remove the currently playing and the previous track from the shuffle history.
                // The upcoming track will be added right back to the history further down below in this method.
                m_shuffleHistory.removeLast();
                m_shuffleHistory.removeLast();
            }
        }

        m_proxyModel.data()->setCurrentIndex( m_proxyModel.data()->mapFromSource( item->index ) );
        m_shuffleHistory << queryAt( index );
        m_shuffleCache = QPersistentModelIndex();
    }

    PlaylistInterface::setCurrentIndex( index );
}


qint64
PlayableProxyModelPlaylistInterface::siblingIndex( int itemsAway, qint64 rootIndex ) const
{
    if ( m_proxyModel.isNull() )
        return -1;

    PlayableProxyModel* proxyModel = m_proxyModel.data();

    while ( m_shuffleHistory.count() && m_shuffleHistory.count() >= proxyModel->rowCount() )
    {
        m_shuffleHistory.removeFirst();
    }

    QModelIndex idx = QModelIndex();
    if ( proxyModel->rowCount() )
    {
        if ( m_shuffled )
        {
            if ( itemsAway < 0 )
            {
                if ( m_shuffleHistory.count() > 1 )
                {
                    if ( proxyModel->itemFromQuery( m_shuffleHistory.at( m_shuffleHistory.count() - 2 ) ) )
                        idx = proxyModel->mapFromSource( proxyModel->itemFromQuery( m_shuffleHistory.at( m_shuffleHistory.count() - 2 ) )->index );
                }
                else
                    return -1;
            }
            else
            {
                // random mode is enabled
                if ( m_shuffleCache.isValid() )
                {
                    idx = m_shuffleCache;
                }
                else
                {
                    int safetyCounter = 0;
                    PlayableItem* item = 0;
                    do
                    {
                        safetyCounter++;
                        idx = proxyModel->index( qrand() % proxyModel->rowCount(), 0 );
                        item = proxyModel->itemFromIndex( proxyModel->mapToSource( idx ) );
                    }
                    while ( safetyCounter < proxyModel->rowCount() &&
                          ( !item || !item->query()->playable() || m_shuffleHistory.contains( item->query() ) ) );

                    if ( item && item->query()->playable() )
                    {
                        m_shuffleCache = idx;
                        tDebug( LOGVERBOSE ) << "Next shuffled PlaylistItem cached:" << item->query()->toString() << item->query()->results().at( 0 )->url()
                                             << "- after" << safetyCounter << "tries to find a track";
                    }
                    else
                    {
                        tDebug() << Q_FUNC_INFO << "Error finding next shuffled playable track";
                    }
                }
            }
        }
        else
        {
            if ( m_repeatMode == PlaylistModes::RepeatOne )
            {
                idx = proxyModel->currentIndex();
            }
            else
            {
                // random mode is disabled
                if ( rootIndex == -1 )
                {
                    idx = proxyModel->currentIndex();
                }
                else
                {
                    PlayableItem* pitem = reinterpret_cast<PlayableItem*>( (void*)rootIndex );
                    if ( !pitem || !pitem->index.isValid() )
                        return -1;

                    idx = proxyModel->mapFromSource( pitem->index );
                }

                idx = proxyModel->index( idx.row() + itemsAway, 0 );
            }
        }
    }

    if ( !idx.isValid() && m_repeatMode == PlaylistModes::RepeatAll )
    {
        // repeat all tracks
        if ( itemsAway > 0 )
        {
            // reset to first item
            idx = proxyModel->index( 0, 0 );
        }
        else
        {
            // reset to last item
            idx = proxyModel->index( proxyModel->rowCount() - 1, 0 );
        }
    }

    while ( idx.isValid() )
    {
        PlayableItem* item = proxyModel->itemFromIndex( proxyModel->mapToSource( idx ) );
        if ( item )
        {
            return (qint64)( item->index.internalPointer() );
        }

        idx = proxyModel->index( idx.row() + ( itemsAway > 0 ? 1 : -1 ), 0 );
    }

    return -1;
}


Tomahawk::result_ptr
PlayableProxyModelPlaylistInterface::currentItem() const
{
    if ( m_proxyModel.isNull() )
        return result_ptr();

    PlayableProxyModel* proxyModel = m_proxyModel.data();

    PlayableItem* item = proxyModel->itemFromIndex( proxyModel->mapToSource( proxyModel->currentIndex() ) );
    if ( item && !item->query().isNull() && item->query()->playable() )
        return item->query()->results().at( 0 );

    return result_ptr();
}


Tomahawk::query_ptr
PlayableProxyModelPlaylistInterface::queryAt( qint64 index ) const
{
    if ( m_proxyModel.isNull() )
        return query_ptr();

    PlayableItem* item = reinterpret_cast<PlayableItem*>( (void*)index );
    if ( item && item->query() )
        return item->query();

    return query_ptr();
}


Tomahawk::result_ptr
PlayableProxyModelPlaylistInterface::resultAt( qint64 index ) const
{
    if ( m_proxyModel.isNull() )
        return result_ptr();

    PlayableItem* item = reinterpret_cast<PlayableItem*>( (void*)index );
    if ( item && item->result() )
        return item->result();

    return result_ptr();
}


qint64
PlayableProxyModelPlaylistInterface::indexOfResult( const Tomahawk::result_ptr& result ) const
{
    if ( m_proxyModel.isNull() )
        return -1;

    PlayableItem* item = m_proxyModel.data()->itemFromResult( result );
    if ( item )
        return (qint64)( item->index.internalPointer() );

    return -1;
}


qint64
PlayableProxyModelPlaylistInterface::indexOfQuery( const Tomahawk::query_ptr& query ) const
{
    if ( m_proxyModel.isNull() )
        return -1;

    PlayableItem* item = m_proxyModel.data()->itemFromQuery( query );
    if ( item )
        return (qint64)( item->index.internalPointer() );

    return -1;
}
