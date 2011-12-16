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

#include "collectionview.h"

#include <QDragEnterEvent>
#include <QPainter>

#include "collectionproxymodel.h"
#include "trackmodel.h"
#include "widgets/overlaywidget.h"
#include "utils/logger.h"

using namespace Tomahawk;


CollectionView::CollectionView( QWidget* parent )
    : TrackView( parent )
{
    setProxyModel( new CollectionProxyModel( this ) );

    setDragDropMode( QAbstractItemView::DragOnly );
}


CollectionView::~CollectionView()
{
    qDebug() << Q_FUNC_INFO;
}


void
CollectionView::setModel( QAbstractItemModel* model )
{
    Q_UNUSED( model );
    qDebug() << "Explicitly use setTrackModel instead";
    Q_ASSERT( false );
}


void
CollectionView::setTrackModel( TrackModel* model )
{
    TrackView::setTrackModel( model );

    setColumnHidden( TrackModel::Score, true ); // Hide score column per default
    setColumnHidden( TrackModel::Origin, true ); // Hide origin column per default

    setGuid( QString( "collectionview/%1" ).arg( model->columnCount() ) );
    sortByColumn( TrackModel::Artist, Qt::AscendingOrder );

    connect( model, SIGNAL( trackCountChanged( unsigned int ) ), SLOT( onTrackCountChanged( unsigned int ) ) );
}


void
CollectionView::dragEnterEvent( QDragEnterEvent* event )
{
    event->ignore();
}


void
CollectionView::onTrackCountChanged( unsigned int tracks )
{
    if ( tracks == 0 )
    {
        overlay()->setText( tr( "This collection is empty." ) );
        overlay()->show();
    }
    else
        overlay()->hide();
}


bool
CollectionView::jumpToCurrentTrack()
{
    scrollTo( proxyModel()->currentIndex(), QAbstractItemView::PositionAtCenter );
    return true;
}
