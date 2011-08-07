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

#include "queueproxymodel.h"

#include "viewmanager.h"
#include "utils/logger.h"

using namespace Tomahawk;


QueueProxyModel::QueueProxyModel( QObject* parent )
    : PlaylistProxyModel( parent )
{
    qDebug() << Q_FUNC_INFO;

    connect( this, SIGNAL( sourceTrackCountChanged( unsigned int ) ), this, SLOT( onTrackCountChanged( unsigned int ) ) );
}


QueueProxyModel::~QueueProxyModel()
{
}


Tomahawk::result_ptr
QueueProxyModel::siblingItem( int itemsAway )
{
    qDebug() << Q_FUNC_INFO << rowCount( QModelIndex() );

    setCurrentIndex( QModelIndex() );
    Tomahawk::result_ptr res = PlaylistProxyModel::siblingItem( itemsAway );

    removeIndex( currentIndex() );

    return res;
}


void
QueueProxyModel::onTrackCountChanged( unsigned int count )
{
    if ( count == 0 )
        ViewManager::instance()->hideQueue();
}
