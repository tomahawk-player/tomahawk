/*
    Copyright (C) 2011  Leo Franchi <leo.franchi@kdab.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "sipconfigdelegate.h"

#include "sip/SipModel.h"
#include "sip/SipPlugin.h"
#include "utils/tomahawkutils.h"
#include <QApplication>
#include <QPainter>

SipConfigDelegate::SipConfigDelegate( QObject* parent )
    : ConfigDelegateBase ( parent )
{

}

bool
SipConfigDelegate::editorEvent ( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
    return ConfigDelegateBase::editorEvent( event, model, option, index );
}

void
SipConfigDelegate::paint ( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );
    QRect itemRect = opt.rect;
    int top = itemRect.top();
    int mid = itemRect.height() / 2;
    int leftEdge = PADDING;

    // one line bold for account name
    // space below it fro an error
    // checkbox, icon, name, online/offline status, config icon
    QFont name = opt.font;
    name.setPointSize( name.pointSize() + 2 );
    name.setBold( true );

    QFont error = opt.font;
    error.setItalic( true );
    error.setPointSize( error.pointSize() - 2 );

    // draw the background
    const QWidget* w = opt.widget;
    QStyle* style = w ? w->style() : QApplication::style();
    style->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, painter, w );

    // draw checkbox first
    int rectW = 24;
    int pos = ( mid ) - ( rectW / 2 );
    QRect checkRect = QRect( pos, pos + top, rectW, rectW );
    opt.rect = checkRect;
    drawCheckBox( opt, painter, w );

    // draw the icon if it exists
    leftEdge = checkRect.right() + PADDING;
    int iconSize = 24;
    pos = ( mid ) - ( iconSize / 2 );
    if( !index.data( Qt::DecorationRole ).value< QIcon >().isNull() ) {
        QRect prect = QRect( leftEdge, pos + top, iconSize, iconSize );

        painter->save();
        painter->drawPixmap( prect, index.data( Qt::DecorationRole ).value< QIcon >().pixmap( prect.size() ) );
        painter->restore();
    }

    // from the right edge--config status and online/offline
    QRect confRect = QRect( itemRect.width() - iconSize - 2 * PADDING, mid - iconSize / 2 + top, iconSize, iconSize );
    if( index.data( SipModel::HasConfig ).toBool() ) {

        QStyleOptionToolButton topt;
        topt.rect = confRect;
        topt.pos = confRect.topLeft();

        drawConfigWrench( painter, opt, topt );
    }

    // draw the online/offline status
    int statusIconSize = 10;
    int statusX = confRect.left() - 2*PADDING - statusIconSize;
    QFont statusF = opt.font;
    statusF.setPointSize( statusF.pointSize() - 2 );
    QFontMetrics statusFM( statusF );

    QPixmap p;
    QString statusText;
    if( index.data( SipModel::ConnectionStateRole ).toInt() == SipPlugin::Connected ) {
        p = QPixmap( RESPATH "images/sipplugin-online.png" );
        statusText = tr( "Online" );
    } else {
        p = QPixmap( RESPATH "images/sipplugin-offline.png" );
        statusText = tr( "Offline" );
    }
    p = p.scaled( statusIconSize, statusIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation );
    painter->drawPixmap( statusX, mid - statusIconSize / 2 + top, statusIconSize, statusIconSize, p );
    int width = statusFM.width( statusText );
    statusX = statusX - PADDING - width;
    painter->save();
    painter->setFont( statusF );
    painter->drawText( QRect( statusX, mid - statusFM.height() / 2 + top, width, statusFM.height() ), statusText );
    painter->restore();

    // name
    painter->save();
    leftEdge = leftEdge + iconSize + PADDING;
    QFontMetrics namefm( name );
    int nameHeight = namefm.boundingRect( "test" ).height();
    // pos will the top-left point of the text rect
    pos = mid - ( nameHeight / 2 );
    // TODO bound with config icon and offline/online status
    width = itemRect.width() - leftEdge;

    if( !index.data( SipModel::ErrorString ).toString().isEmpty() ) { // error, show that too
        QRect errorRect( leftEdge, mid + top, width, mid - PADDING );

        QFontMetrics errorFm( error );
        QString str = errorFm.elidedText( index.data( SipModel::ErrorString ).toString(), Qt::ElideRight, errorRect.width() );
        painter->setFont( error );
        painter->drawText( errorRect, str );

        pos = mid - errorRect.height() - 2; // move the name rect up
    }
    QString nameStr = namefm.elidedText( index.data( Qt::DisplayRole ).toString(), Qt::ElideRight, width );
    painter->setFont( name );
    painter->drawText( QRect( leftEdge, pos + top, width, nameHeight ), nameStr );
    painter->restore();
}
