/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "ResolverConfigDelegate.h"

#include "ExternalResolverGui.h"

#include "utils/Logger.h"

#include <QApplication>
#include <QPainter>
#include <QMouseEvent>

#define PADDING 4
#define ICONSIZE 24


ResolverConfigDelegate::ResolverConfigDelegate( QObject* parent )
    : ConfigDelegateBase( parent )
{
    connect( this, SIGNAL( configPressed( QModelIndex ) ), this, SLOT( onConfigPressed( QModelIndex ) ) );
}

void
ResolverConfigDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );
    QRect itemRect = opt.rect;
    int top = itemRect.top();

    QFont name = opt.font;
    name.setPointSize( name.pointSize() + 2 );
    name.setBold( true );

    QFont path = opt.font;
    path.setItalic( true );
    path.setPointSize( path.pointSize() - 1 );

    const bool error =  (Tomahawk::ExternalResolver::ErrorState)index.data( ResolversModel::ErrorState ).toInt() != Tomahawk::ExternalResolver::NoError;
    const bool fileNotFound = (Tomahawk::ExternalResolver::ErrorState)index.data( ResolversModel::ErrorState ).toInt() == Tomahawk::ExternalResolver::FileNotFound;
    const bool failedToLoad = (Tomahawk::ExternalResolver::ErrorState)index.data( ResolversModel::ErrorState ).toInt() == Tomahawk::ExternalResolver::FailedToLoad;

    QFontMetrics bfm( name );
    QFontMetrics sfm( path );

    // draw the background
    const QWidget* w = opt.widget;
    QStyle* style = w ? w->style() : QApplication::style();
    style->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, painter, w );

    int rightSplit = itemRect.width();
    QRect confRect = QRect( rightSplit - ICONSIZE - 2 * PADDING, 2 * PADDING + top, ICONSIZE, ICONSIZE );

    // if the Resolver.has a config widget, paint it first (right-aligned)
    if( index.data( ResolversModel::HasConfig ).toBool() ) {
        QStyleOptionToolButton topt;
        topt.rect = confRect;
        topt.pos = confRect.topLeft();

        drawConfigWrench( painter, opt, topt );
    }

    // draw check
    QRect checkRect = confRect;
    checkRect.moveTo( 2 * PADDING, 2 * PADDING + top );
    opt.rect = checkRect;
    drawCheckBox( opt, painter, w );
    itemRect.setX( opt.rect.topRight().x() + PADDING );

    painter->save();
    painter->setFont( name );
    QRect textRect = itemRect.adjusted( PADDING, PADDING, -PADDING, -PADDING );

    if( index.data( ResolversModel::HasConfig ).toBool() )
        textRect.setRight( confRect.topLeft().x() - PADDING );

    textRect.setBottom( itemRect.height() / 2 + top  );
    QString nameStr = bfm.elidedText( index.data( ResolversModel::ResolverName ).toString(),Qt::ElideRight, textRect.width() );
    painter->drawText( textRect, nameStr );
    painter->restore();

    painter->save();
    painter->setFont( path );
    QString pathStr = index.data( ResolversModel::ResolverPath ).toString();
    if( error )
    {
        painter->setPen( QColor( Qt::red ).lighter( 150 ) );
    } else {
        painter->setPen( Qt::gray );
    }

    if( fileNotFound ) {
        pathStr = tr( "Not found: %1" ).arg( pathStr );
    } else if ( failedToLoad )
    {
        pathStr = tr( "Failed to load: %1" ).arg( pathStr );
    }

    textRect.moveTop(  itemRect.height() / 2 + top );
    pathStr = sfm.elidedText( pathStr, Qt::ElideMiddle, textRect.width() );
    painter->drawText( textRect, pathStr );
    painter->restore();

}

QRect
ResolverConfigDelegate::configRectForIndex( const QStyleOptionViewItem& option, const QModelIndex& idx ) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, idx );
    QRect itemRect = opt.rect;
    int top = itemRect.top();

    QRect confRect = QRect( itemRect.width() - ICONSIZE - 2 * PADDING, 2 * PADDING + top, ICONSIZE, ICONSIZE );
    return confRect;
}

QRect
ResolverConfigDelegate::checkRectForIndex( const QStyleOptionViewItem &option, const QModelIndex &idx, int role ) const
{
    Q_UNUSED( role );

    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, idx );
    QRect itemRect = opt.rect;
    int top = itemRect.top();

    QRect checkRect = QRect(2 * PADDING, 2 * PADDING + top, ICONSIZE, ICONSIZE );
    return checkRect;
}

void
ResolverConfigDelegate::onConfigPressed( const QModelIndex& idx )
{
    emit openConfig( idx.data( ResolversModel::ResolverPath ).toString() );
}
