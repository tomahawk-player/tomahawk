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

#include "queueproxymodelplaylistinterface.h"

#include "queueproxymodel.h"
#include "utils/logger.h"

using namespace Tomahawk;

QueueProxyModelPlaylistInterface::QueueProxyModelPlaylistInterface( QueueProxyModel *proxyModel )
    : PlaylistProxyModelPlaylistInterface( proxyModel )
{
}


QueueProxyModelPlaylistInterface::~QueueProxyModelPlaylistInterface()
{
    m_proxyModel.clear();
}


Tomahawk::result_ptr
QueueProxyModelPlaylistInterface::siblingItem( int itemsAway )
{
    if ( m_proxyModel.isNull() )
        return Tomahawk::result_ptr();

    m_proxyModel.data()->setCurrentIndex( QModelIndex() );
    Tomahawk::result_ptr res = PlaylistProxyModelPlaylistInterface::siblingItem( itemsAway );

    m_proxyModel.data()->remove( m_proxyModel.data()->currentIndex() );

    return res;
}
