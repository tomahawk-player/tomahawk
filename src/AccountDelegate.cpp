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


#include "AccountDelegate.h"

#include <QApplication>
#include <QPainter>

#include "accounts/AccountModel.h"
#include "accounts/Account.h"

#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#define ICONSIZE 36
#define WRENCH_SIZE 24
#define STATUS_ICON_SIZE 18
#define CHECK_LEFT_EDGE 8

using namespace Tomahawk;
using namespace Accounts;

AccountDelegate::AccountDelegate( QObject* parent )
    : ConfigDelegateBase ( parent )
{
    connect( this, SIGNAL( configPressed( QModelIndex ) ), this, SLOT( askedForEdit( QModelIndex ) ) );
}

bool
AccountDelegate::editorEvent ( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
    return ConfigDelegateBase::editorEvent( event, model, option, index );
}

void
AccountDelegate::paint ( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );
    const QRect itemRect = opt.rect;
    const int top = itemRect.top();
    const int mid = itemRect.height() / 2;

    // one line bold for account name
    // space below it for account description
    // checkbox, icon, name, online/offline status, config icon
    QFont name = opt.font;
    name.setPointSize( name.pointSize() + 2 );
    name.setBold( true );

    QFont desc = opt.font;
    desc.setItalic( true );
    desc.setPointSize( desc.pointSize() - 2 );

    // draw the background
    const QWidget* w = opt.widget;
    QStyle* style = w ? w->style() : QApplication::style();
    style->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, painter, w );

    int iconLeftEdge = CHECK_LEFT_EDGE + ICONSIZE + PADDING;
    int textLeftEdge = iconLeftEdge + ICONSIZE + PADDING;

    // draw checkbox first
    int pos = ( mid ) - ( WRENCH_SIZE / 2 );
    QRect checkRect = QRect( CHECK_LEFT_EDGE, pos + top, WRENCH_SIZE, WRENCH_SIZE );
    opt.rect = checkRect;
    drawCheckBox( opt, painter, w );

    // draw the icon if it exists
    pos = mid - ( ICONSIZE / 2 );
    if( !index.data( Qt::DecorationRole ).value< QIcon >().isNull() ) {
        QRect prect = QRect( iconLeftEdge, pos + top, ICONSIZE, ICONSIZE );

        painter->save();
        painter->drawPixmap( prect, index.data( Qt::DecorationRole ).value< QIcon >().pixmap( prect.size() ) );
        painter->restore();
    }

    // from the right edge--config status and online/offline
    QRect confRect = QRect( itemRect.width() - WRENCH_SIZE - 2 * PADDING, mid - WRENCH_SIZE / 2 + top, WRENCH_SIZE, WRENCH_SIZE );
    if( index.data( AccountModel::HasConfig ).toBool() ) {

        QStyleOptionToolButton topt;
        topt.rect = confRect;
        topt.pos = confRect.topLeft();

        drawConfigWrench( painter, opt, topt );
    }


    // draw the online/offline status
    const bool hasCapability = ( static_cast< AccountModel::BasicCapabilities >( index.data( AccountModel::BasicCapabilityRole ).toInt() ) != AccountModel::NoCapabilities );
    const int quarter = mid - ( itemRect.height() / 4 );
    const int statusY = hasCapability ? quarter : mid;
    const int statusX = confRect.left() - 2*PADDING - STATUS_ICON_SIZE;

    QFont statusF = opt.font;
    statusF.setPointSize( statusF.pointSize() - 2 );
    QFontMetrics statusFM( statusF );

    QPixmap p;
    QString statusText;
    Account::ConnectionState state = static_cast< Account::ConnectionState >( index.data( AccountModel::ConnectionStateRole ).toInt() );
    if( state == Account::Connected ) {
        p = QPixmap( RESPATH "images/sipplugin-online.png" );
        statusText = tr( "Online" );
    } else if( state == Account::Connecting ) {
        p = QPixmap( RESPATH "images/sipplugin-offline.png" );
        statusText = tr( "Connecting..." );
    } else {
        p = QPixmap( RESPATH "images/sipplugin-offline.png" );
        statusText = tr( "Offline" );
    }
    p = p.scaled( STATUS_ICON_SIZE, STATUS_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation );
    painter->drawPixmap( statusX, statusY - STATUS_ICON_SIZE / 2 + top, STATUS_ICON_SIZE, STATUS_ICON_SIZE, p );
    int width = statusFM.width( statusText );
    int statusTextX = statusX - PADDING - width;
    painter->save();
    painter->setFont( statusF );
    painter->drawText( QRect( statusTextX, statusY - statusFM.height() / 2 + top, width, statusFM.height() ), statusText );

    // draw optional capability text if it exists
    if ( hasCapability )
    {
        QString capString;
        AccountModel::BasicCapabilities cap = static_cast< AccountModel::BasicCapabilities >( index.data( AccountModel::BasicCapabilityRole ).toInt() );
        if ( ( cap & AccountModel::SipCapability ) && ( cap & AccountModel::ResolverCapability ) )
            capString = tr( "Connect to and play from friends" );
        else if ( cap & AccountModel::SipCapability )
            capString = tr( "Connect to friends" );
        else if ( cap & AccountModel::ResolverCapability )
            capString = tr( "Find Music");

        // checkbox for capability
        const int capY = statusY + ( itemRect.height() / 2 );
        QRect capCheckRect( statusX, capY - STATUS_ICON_SIZE / 2 + top, STATUS_ICON_SIZE, STATUS_ICON_SIZE );
        opt.rect = capCheckRect;
        drawCheckBox( opt, painter, w );

        // text to accompany checkbox
        const int capW = statusFM.width( capString );
        const int capTextX = statusX - PADDING  - capW;
        painter->drawText( QRect( capTextX, capY - statusFM.height() / 2 + top, capW, statusFM.height() ), capString );

        if ( capTextX < statusTextX )
            statusTextX = capTextX;
    }
    painter->restore();

    // name
    painter->save();
    painter->setFont( name );
    QFontMetrics namefm( name );
    // pos will the top-left point of the text rect
    pos = mid - ( namefm.height() / 2 ) + top;
    // TODO bound with config icon and offline/online status
    width = itemRect.width() - statusTextX;
    QRect nameRect( textLeftEdge, pos, width, namefm.height() );
    painter->drawText( nameRect, index.data( AccountModel::AccountName ).toString() );

    nameRect.translate( mid, 0 ); // move down by half the hight
    painter->drawText( nameRect, index.data( AccountModel::DescText ).toString() );
    painter->restore();

}

QRect
AccountDelegate::checkRectForIndex( const QStyleOptionViewItem &option, const QModelIndex &idx, int role ) const
{
    if ( role == Qt::CheckStateRole )
    {
        // the whole resolver checkbox
        QStyleOptionViewItemV4 opt = option;
        initStyleOption( &opt, idx );
        const int mid = opt.rect.height() / 2;
        const int pos = mid - ( ICONSIZE / 2 );
        QRect checkRect( CHECK_LEFT_EDGE, pos + opt.rect.top(), ICONSIZE, ICONSIZE );

        return checkRect;
    } else if ( role == AccountModel::BasicCapabilityRole )
    {
        // The capabilities checkbox
        QStyleOptionViewItemV4 opt = option;
        initStyleOption( &opt, idx );
        const int quarter = opt.rect.height() / 4 + opt.rect.height()  / 2;
        const int leftEdge = opt.rect.width() - PADDING - WRENCH_SIZE - PADDING - WRENCH_SIZE;
        QRect checkRect( leftEdge, quarter, WRENCH_SIZE, WRENCH_SIZE );
        return checkRect;
    }
    return QRect();
}

QRect
AccountDelegate::configRectForIndex( const QStyleOptionViewItem& option, const QModelIndex& idx ) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, idx );
    QRect itemRect = opt.rect;
    QRect confRect = QRect( itemRect.width() - ICONSIZE - 2 * PADDING, (opt.rect.height() / 2) - ICONSIZE / 2 + opt.rect.top(), ICONSIZE, ICONSIZE );
    return confRect;
}


QSize
AccountDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    return ConfigDelegateBase::sizeHint( option, index );
}

void
AccountDelegate::askedForEdit( const QModelIndex& idx )
{
    emit openConfig( qobject_cast< Account* >( idx.data( AccountModel::AccountData ).value< QObject* >() ) );
}


