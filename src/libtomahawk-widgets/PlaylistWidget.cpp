/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "PlaylistWidget.h"

#include "utils/AnimatedSpinner.h"

using namespace Tomahawk;
using namespace Tomahawk::Widgets;

PlaylistWidget::PlaylistWidget( QWidget* parent )
    : QListView( parent )
{
    m_overlay = new OverlayWidget( this );
    new LoadingSpinner( this );
}


void
PlaylistWidget::setModel( QAbstractItemModel* model )
{
    QListView::setModel( model );

    connect( model, SIGNAL( modelReset() ), SLOT( verifySize() ) );
    connect( model, SIGNAL( rowsInserted( QModelIndex, int, int ) ), SLOT( verifySize() ) );
    connect( model, SIGNAL( rowsRemoved( QModelIndex, int, int ) ), SLOT( verifySize() ) );

    emit modelChanged();
    verifySize();
}


void
PlaylistWidget::verifySize()
{
    if ( !model() )
        return;

    if ( model()->rowCount() > 0 )
        setFixedHeight( model()->rowCount() * itemDelegate()->sizeHint( QStyleOptionViewItem(), model()->index( 0, 0 ) ).height() + frameWidth() * 2 );
}
