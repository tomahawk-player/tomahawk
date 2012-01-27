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

#define ICONSIZE 34
#define WRENCH_SIZE 24
#define STATUS_ICON_SIZE 13
#define CHECK_LEFT_EDGE 8

using namespace Tomahawk;
using namespace Accounts;

AccountDelegate::AccountDelegate( QObject* parent )
    : ConfigDelegateBase ( parent )
{
    connect( this, SIGNAL( configPressed( QModelIndex ) ), this, SLOT( askedForEdit( QModelIndex ) ) );

    m_cachedIcons[ "sipplugin-online" ] = QPixmap( RESPATH "images/sipplugin-online.png" ).scaled( STATUS_ICON_SIZE, STATUS_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation );
    m_cachedIcons[ "sipplugin-offline" ] = QPixmap( RESPATH "images/sipplugin-offline.png" ).scaled( STATUS_ICON_SIZE, STATUS_ICON_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation );

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
    const int quarter = mid - ( itemRect.height() / 4 );

    // one line bold for account name
    // space below it for online/offline status
    // checkbox, icon, name/status, features, config icon
    QFont name = opt.font;
    name.setPointSize( name.pointSize() + 2 );
    name.setBold( true );

    QFont smallFont = opt.font;
    smallFont.setPointSize( smallFont.pointSize() - 1 );
    QFontMetrics smallFontFM( smallFont );

    // draw the background
    const QWidget* w = opt.widget;
    QStyle* style = w ? w->style() : QApplication::style();
    style->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, painter, w );

    int iconLeftEdge = CHECK_LEFT_EDGE + WRENCH_SIZE + PADDING;
    int textLeftEdge = iconLeftEdge + ICONSIZE + PADDING;

    // draw checkbox first
    int pos = ( mid ) - ( WRENCH_SIZE / 2 );
    QRect checkRect = QRect( CHECK_LEFT_EDGE, pos + top, WRENCH_SIZE, WRENCH_SIZE );
    opt.rect = checkRect;
    drawCheckBox( opt, painter, w );

    // draw the icon if it exists
    pos = mid - ( ICONSIZE / 2 );
    if( !index.data( Qt::DecorationRole ).value< QPixmap >().isNull() ) {
        QRect prect = QRect( iconLeftEdge, pos + top, ICONSIZE, ICONSIZE );

        painter->save();
        painter->drawPixmap( prect, index.data( Qt::DecorationRole ).value< QPixmap >().scaled( prect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation ) );
        painter->restore();
    }

    // name
    painter->save();
    painter->setFont( name );
    QFontMetrics namefm( name );
    // pos will the top-left point of the text rect
    pos = quarter - ( namefm.height() / 2 ) + top;
    const QString nameStr = index.data( AccountModel::AccountName ).toString();
    const int titleWidth = namefm.width( nameStr );
    const QRect nameRect( textLeftEdge, pos, titleWidth, namefm.height() );
    painter->drawText( nameRect, nameStr );
    painter->restore();

    // draw the online/offline status
    const int stateY = mid + quarter - ( smallFontFM.height() / 2 ) + top;

    QPixmap p;
    QString statusText;
    Account::ConnectionState state = static_cast< Account::ConnectionState >( index.data( AccountModel::ConnectionStateRole ).toInt() );
    if ( state == Account::Connected )
    {
        p = m_cachedIcons[ "sipplugin-online" ];
        statusText = tr( "Online" );
    }
    else if ( state == Account::Connecting )
    {
        p = m_cachedIcons[ "sipplugin-offline" ];
        statusText = tr( "Connecting..." );
    }
    else
    {
        p = m_cachedIcons[ "sipplugin-offline" ];
        statusText = tr( "Offline" );
    }
    painter->drawPixmap( textLeftEdge, stateY, STATUS_ICON_SIZE, STATUS_ICON_SIZE, p );

    int width = smallFontFM.width( statusText );
    int statusTextX = textLeftEdge + STATUS_ICON_SIZE + PADDING;
    painter->save();
    painter->setFont( smallFont );
    painter->drawText( QRect( statusTextX, stateY, width, smallFontFM.height() ), statusText );
    painter->restore();

    // right-most edge of text on left (name, desc) is the cutoff point for the rest of the delegate
    width = qMax( statusTextX + width, textLeftEdge + titleWidth );

    // from the right edge--config status and online/offline
    QRect confRect = QRect( itemRect.width() - WRENCH_SIZE - 2 * PADDING, mid - WRENCH_SIZE / 2 + top, WRENCH_SIZE, WRENCH_SIZE );
    if( index.data( AccountModel::HasConfig ).toBool() ) {

        QStyleOptionToolButton topt;
        topt.rect = confRect;
        topt.pos = confRect.topLeft();

        drawConfigWrench( painter, opt, topt );
    }

    const bool hasCapability = ( static_cast< Accounts::AccountTypes >( index.data( AccountModel::AccountTypeRole ).toInt() ) != Accounts::NoType );

    // draw optional capability text if it exists
    if ( hasCapability )
    {
        QString capString;
        AccountTypes types = AccountTypes( index.data( AccountModel::AccountTypeRole ).toInt() );
        if ( ( types & Accounts::SipType ) && ( types & Accounts::ResolverType ) )
            capString = tr( "Connects to, plays from friends" );
        else if ( types & Accounts::SipType )
            capString = tr( "Connects to friends" );
        else if ( types & Accounts::ResolverType )
            capString = tr( "Finds Music");

        // checkbox for capability
//         QRect capCheckRect( statusX, capY - STATUS_ICON_SIZE / 2 + top, STATUS_ICON_SIZE, STATUS_ICON_SIZE );
//         opt.rect = capCheckRect;
//         drawCheckBox( opt, painter, w );

        // text to accompany checkbox
        const int capY = mid - ( smallFontFM.height() / 2 ) + top;
        const int configLeftEdge = confRect.left() - PADDING;
        const int capW = configLeftEdge - width;
        // Right-align text
        const int capTextX = qMax( width, configLeftEdge - smallFontFM.width( capString ) );
        painter->setFont( smallFont );
        painter->drawText( QRect( capTextX, capY, configLeftEdge - capTextX, smallFontFM.height() ), Qt::AlignRight, capString );
    }
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
    } else if ( role == AccountModel::AccountTypeRole )
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


