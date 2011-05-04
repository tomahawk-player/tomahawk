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

#define ICONSIZE 24
SipConfigDelegate::SipConfigDelegate( QObject* parent )
    : ConfigDelegateBase ( parent )
{
    connect( this, SIGNAL( configPressed( QModelIndex ) ), this, SLOT( askedForEdit( QModelIndex ) ) );
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

    int checkLeftEdge = 8;
    int iconLeftEdge = checkLeftEdge + ICONSIZE + PADDING;
    int textLeftEdge = iconLeftEdge + ICONSIZE + PADDING;

    if( index.data( SipModel::FactoryRole ).toBool() ) { // this is the "add new account" row
        // draw a border and background
        painter->save();
        painter->setRenderHints( QPainter::Antialiasing );
        painter->setBrush( QApplication::palette().color( QPalette::Active, QPalette::Highlight ).lighter( 150 ) );
        QPainterPath roundedRect;
        roundedRect.addRoundedRect( itemRect.adjusted( 1, 1, -1, -1 ), 3, 3 );
        painter->drawPath( roundedRect );
        painter->setBrush( QApplication::palette().color( QPalette::Active, QPalette::Highlight ).lighter( 170 ) );
        painter->fillPath( roundedRect, painter->brush() );
        painter->restore();

        // draw "+" icon in checkbox column
        int rectW = 18;
        int diff = ( ICONSIZE/ 2 ) - ( rectW / 2) ;
        int pos = ( mid ) - ( rectW / 2 );
        QRect plusRect = QRect( checkLeftEdge + diff, pos + top, rectW, rectW );
        QPixmap p( RESPATH "images/list-add.png" );
        painter->drawPixmap( plusRect, p );

        // draw text
        QFont f = opt.font;
        f.setPointSize( f.pointSize() );
        f.setBold( true );
        QFontMetrics fm( f );
        QString text = index.data( Qt::DisplayRole ).toString();
        QRect textR = fm.boundingRect( text );
        textR.moveLeft( textLeftEdge );
        textR.moveTop( mid - ( textR.height() / 2 ) + top );
        textR.setRight( itemRect.right() );
        painter->setFont( f );
        painter->drawText( textR, text );
    } else if( index.data( SipModel::FactoryItemRole ).toBool() ) { // this is an account type

//         ConfigDelegateBase::paint( painter, opt, index );
//         int indent = 10;
        // draw a border and background
        painter->save();
        painter->setRenderHints( QPainter::Antialiasing );
        painter->setBrush( QApplication::palette().color( QPalette::Active, QPalette::Highlight ).lighter( 170 ) );
        QPainterPath roundedRect;
        roundedRect.addRoundedRect( itemRect.adjusted( 1, 1, -1, -1 ), 3, 3 );
        painter->drawPath( roundedRect );
        painter->setBrush( QApplication::palette().color( QPalette::Active, QPalette::Highlight ).lighter( 180 ) );
        painter->fillPath( roundedRect, painter->brush() );
        painter->restore();

        QIcon icon = index.data( SipModel::FactoryItemIcon ).value< QIcon >();
        if( !icon.isNull() ) {
            int rectW = 18;
            int diff = ( ICONSIZE/ 2 ) - ( rectW / 2) ;
            int pos = ( mid ) - ( rectW / 2 );
            QRect rect = QRect( checkLeftEdge + diff, pos + top, rectW, rectW );
            QPixmap p( icon.pixmap( rect.size() ) );
            painter->drawPixmap( rect, p );
        }

        // draw text
        QFont f = opt.font;
        f.setPointSize( f.pointSize() );
        f.setBold( true );
        QFontMetrics fm( f );
        QString text = index.data( Qt::DisplayRole ).toString();
        QRect textR = fm.boundingRect( text );
        textR.moveLeft( textLeftEdge );
        textR.moveTop( mid - ( textR.height() / 2 ) + top );
        textR.setRight( itemRect.right() );
        painter->setFont( f );
        painter->drawText( textR, text );
    } else { // this is an existing account to show
        // draw checkbox first
        int pos = ( mid ) - ( ICONSIZE / 2 );
        QRect checkRect = QRect( checkLeftEdge, pos + top, ICONSIZE, ICONSIZE );
        opt.rect = checkRect;
        drawCheckBox( opt, painter, w );

        // draw the icon if it exists
        pos = ( mid ) - ( ICONSIZE / 2 );
        if( !index.data( Qt::DecorationRole ).value< QIcon >().isNull() ) {
            QRect prect = QRect( iconLeftEdge, pos + top, ICONSIZE, ICONSIZE );

            painter->save();
            painter->drawPixmap( prect, index.data( Qt::DecorationRole ).value< QIcon >().pixmap( prect.size() ) );
            painter->restore();
        }

        // from the right edge--config status and online/offline
        QRect confRect = QRect( itemRect.width() - ICONSIZE - 2 * PADDING, mid - ICONSIZE / 2 + top, ICONSIZE, ICONSIZE );
        if( index.data( SipModel::HasConfig ).toBool() ) {

            QStyleOptionToolButton topt;
            topt.rect = confRect;
            topt.pos = confRect.topLeft();

            drawConfigWrench( painter, opt, topt );
        }

        // draw the online/offline status
        int statusIconSize = 18;
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
        QFontMetrics namefm( name );
        int nameHeight = namefm.boundingRect( "test" ).height();
        // pos will the top-left point of the text rect
        pos = mid - ( nameHeight / 2 );
        // TODO bound with config icon and offline/online status
        width = itemRect.width() - textLeftEdge;

        if( !index.data( SipModel::ErrorString ).toString().isEmpty() ) { // error, show that too
            QRect errorRect( textLeftEdge, mid + top, width, mid - PADDING + 1 );

            QFontMetrics errorFm( error );
            QString str = errorFm.elidedText( index.data( SipModel::ErrorString ).toString(), Qt::ElideRight, errorRect.width() );
            painter->setFont( error );
            painter->drawText( errorRect, str );

            pos = mid - errorRect.height() - 2; // move the name rect up
        }
        QString nameStr = namefm.elidedText( index.data( Qt::DisplayRole ).toString(), Qt::ElideRight, width );
        painter->setFont( name );
        painter->drawText( QRect( textLeftEdge, pos + top, width, nameHeight + 1 ), nameStr );
        painter->restore();
    }
}

QRect
SipConfigDelegate::configRectForIndex( const QStyleOptionViewItem& option, const QModelIndex& idx ) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, idx );
    QRect itemRect = opt.rect;
    QRect confRect = QRect( itemRect.width() - ICONSIZE - 2 * PADDING, (opt.rect.height() / 2) - ICONSIZE / 2 + opt.rect.top(), ICONSIZE, ICONSIZE );
    return confRect;
}


QSize
SipConfigDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    if( index.data( SipModel::FactoryRole ).toBool() || index.data( SipModel::FactoryItemRole ).toBool() ) { // this is the "add new account" row
        // enough space for one line of text
        QStyleOptionViewItemV4 opt = option;
        initStyleOption( &opt, index );
        int width = QStyledItemDelegate::sizeHint( option, index ).width();

        QFont name = opt.font;
        name.setPointSize( name.pointSize() + 1 );
        name.setBold( true );
        QFontMetrics sfm( name );
        return QSize( width, 3 * PADDING + sfm.height() );
    } else { // this is an existing account to show
        return ConfigDelegateBase::sizeHint( option, index );
    }
}

void
SipConfigDelegate::askedForEdit( const QModelIndex& idx )
{
    emit openConfig( qobject_cast< SipPlugin* >( idx.data( SipModel::SipPluginData ).value< QObject* >() ) );
}


