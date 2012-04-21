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

#include "albumproxymodelplaylistinterface.h"

#include "albumproxymodel.h"
#include "Artist.h"
#include "albumitem.h"
#include "Query.h"
#include "utils/logger.h"

using namespace Tomahawk;

AlbumProxyModelPlaylistInterface::AlbumProxyModelPlaylistInterface( AlbumProxyModel *proxyModel )
    : Tomahawk::PlaylistInterface()
    , m_proxyModel( proxyModel )
    , m_repeatMode( PlaylistInterface::NoRepeat )
    , m_shuffled( false )
{
}


AlbumProxyModelPlaylistInterface::~AlbumProxyModelPlaylistInterface()
{
    m_proxyModel.clear();
}


QList< Tomahawk::query_ptr >
AlbumProxyModelPlaylistInterface::tracks()
{
    Q_ASSERT( FALSE );
    QList<Tomahawk::query_ptr> queries;
    return queries;
}


int
AlbumProxyModelPlaylistInterface::unfilteredTrackCount() const
{
    return ( m_proxyModel.isNull() ? 0 : m_proxyModel.data()->sourceModel()->rowCount( QModelIndex() ) );
}


int
AlbumProxyModelPlaylistInterface::trackCount() const
{
    return ( m_proxyModel.isNull() ? 0 : m_proxyModel.data()->rowCount( QModelIndex() ) );
}


Tomahawk::result_ptr
AlbumProxyModelPlaylistInterface::currentItem() const
{
     return Tomahawk::result_ptr();
}


QString
AlbumProxyModelPlaylistInterface::filter() const
{
    return ( m_proxyModel.isNull() ? QString() : m_proxyModel.data()->filterRegExp().pattern() );
}


void
AlbumProxyModelPlaylistInterface::setFilter( const QString& pattern )
{
    qDebug() << Q_FUNC_INFO;

    if ( m_proxyModel.isNull() )
        return;

    m_proxyModel.data()->setFilterRegExp( pattern );
    m_proxyModel.data()->emitFilterChanged( pattern );
}


Tomahawk::result_ptr
AlbumProxyModelPlaylistInterface::siblingItem( int itemsAway )
{
    Q_UNUSED( itemsAway );
    qDebug() << Q_FUNC_INFO;
    return Tomahawk::result_ptr( 0 );
}
