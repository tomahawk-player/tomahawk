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
}


PlayableProxyModelPlaylistInterface::~PlayableProxyModelPlaylistInterface()
{
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
PlayableProxyModelPlaylistInterface::tracks()
{
    if ( m_proxyModel.isNull() )
        return QList< Tomahawk::query_ptr >();

    PlayableProxyModel* proxyModel = m_proxyModel.data();
    QList<Tomahawk::query_ptr> queries;

    for ( int i = 0; i < proxyModel->rowCount( QModelIndex() ); i++ )
    {
        PlayableItem* item = proxyModel->itemFromIndex( proxyModel->mapToSource( proxyModel->index( i, 0 ) ) );
        if ( item )
            queries << item->query();
    }

    return queries;
}


Tomahawk::result_ptr
PlayableProxyModelPlaylistInterface::siblingItem( int itemsAway )
{
    return siblingItem( itemsAway, false );
}


bool
PlayableProxyModelPlaylistInterface::hasNextItem()
{
    return !( siblingItem( 1, true ).isNull() );
}


Tomahawk::result_ptr
PlayableProxyModelPlaylistInterface::siblingItem( int itemsAway, bool readOnly )
{
    tDebug( LOGVERBOSE ) << Q_FUNC_INFO << m_shuffled << itemsAway << readOnly;

    if ( m_proxyModel.isNull() )
        return Tomahawk::result_ptr();

    PlayableProxyModel* proxyModel = m_proxyModel.data();

    while ( m_shuffleHistory.count() >= proxyModel->rowCount() )
    {
        m_shuffleHistory.removeFirst();
    }

    QModelIndex idx = proxyModel->index( 0, 0 );
    if ( proxyModel->rowCount() )
    {
        if ( m_shuffled )
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
                    if ( readOnly )
                    {
                        m_shuffleCache = idx;
                        tDebug( LOGVERBOSE ) << "Next shuffled PlaylistItem cached:" << item->query()->toString() << item->query()->results().at( 0 )->url()
                                             << "- after" << safetyCounter << "tries to find a track";
                    }
                }
                else
                {
                    tDebug() << Q_FUNC_INFO << "Error finding next shuffled playable track";
                }
            }
        }
        else if ( proxyModel->currentIndex().isValid() )
        {
            // random mode is disabled
            idx = proxyModel->currentIndex();

            if ( m_repeatMode != PlaylistModes::RepeatOne )
            {
                // keep progressing through the playlist normally
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

    // Try to find the next available PlaylistItem (with results)
    while ( idx.isValid() )
    {
        PlayableItem* item = proxyModel->itemFromIndex( proxyModel->mapToSource( idx ) );
        if ( item && item->query()->playable() )
        {
            tDebug( LOGVERBOSE ) << "Next PlaylistItem found:" << item->query()->toString() << item->query()->results().at( 0 )->url();
            if ( !readOnly )
            {
                proxyModel->setCurrentIndex( idx );
                m_shuffleHistory << item->query();
                m_shuffleCache = QPersistentModelIndex();
            }

            return item->query()->results().at( 0 );
        }

        idx = proxyModel->index( idx.row() + ( itemsAway > 0 ? 1 : -1 ), 0 );
    }

    if ( !readOnly )
        proxyModel->setCurrentIndex( QModelIndex() );
    return Tomahawk::result_ptr();
}


Tomahawk::result_ptr
PlayableProxyModelPlaylistInterface::currentItem() const
{
    if ( m_proxyModel.isNull() )
        return Tomahawk::result_ptr();

    PlayableProxyModel* proxyModel = m_proxyModel.data();

    PlayableItem* item = proxyModel->itemFromIndex( proxyModel->mapToSource( proxyModel->currentIndex() ) );
    if ( item && !item->query().isNull() && item->query()->playable() )
        return item->query()->results().at( 0 );

    return Tomahawk::result_ptr();
}

