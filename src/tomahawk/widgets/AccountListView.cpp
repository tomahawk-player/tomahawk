/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013,      Teo Mrnjavac <teo@kde.org>
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

#include "AccountListView.h"
#include "accounts/AccountDelegate.h"

#include <QScrollBar>

AccountListView::AccountListView( QWidget* parent )
    : QListView( parent )
{}

void
AccountListView::wheelEvent( QWheelEvent* e )
{
#ifndef Q_WS_MAC
    //HACK: Workaround for QTBUG-7232: Smooth scrolling (scroll per pixel) in ItemViews
    //      does not work as expected.
    verticalScrollBar()->setSingleStep( ACCOUNT_DELEGATE_ROW_HEIGHT_MULTIPLIER * fontMetrics().height() / 8 );
                                     // ^ scroll step is 1/8 of the estimated row height
#endif

    QListView::wheelEvent( e );
}
