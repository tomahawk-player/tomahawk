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

#include "AccountFactoryWrapperDelegate.h"

#include "AccountFactoryWrapper.h"
#include "Source.h"
#include "accounts/Account.h"
#include "utils/ImageRegistry.h"
#include "utils/TomahawkUtils.h"

#include <QApplication>
#include <QPainter>
#include <QMouseEvent>

using namespace Tomahawk::Accounts;

#define ICON_SIZE 15
#define CONFIG_WRENCH_SIZE 20
#define PADDING 4


AccountFactoryWrapperDelegate::AccountFactoryWrapperDelegate( QObject* parent )
    : QStyledItemDelegate( parent )
{
}


void
AccountFactoryWrapperDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );

    const int center = opt.rect.height() / 2 + opt.rect.top();
    const int topIcon = center - ICON_SIZE/2;

    // draw the background
    const QWidget* w = opt.widget;
    QStyle* style = w ? w->style() : QApplication::style();
    style->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, painter, w );

    Account* acc = qobject_cast< Account* >( index.data( AccountFactoryWrapper::AccountRole ).value< QObject* >() );
    Q_ASSERT( acc );

    // Checkbox on left edge, then text
    const QRect checkRect( PADDING/4, PADDING/4 + opt.rect.top(), opt.rect.height() - PADDING/4, opt.rect.height() - PADDING/4 );
    m_cachedCheckRects[ index ] = checkRect;
    QStyleOptionViewItemV4 opt2 = opt;
    opt2.rect = checkRect;
    opt.checkState == Qt::Checked ? opt2.state |= QStyle::State_On : opt2.state |= QStyle::State_Off;
    style->drawPrimitive( QStyle::PE_IndicatorViewItemCheck, &opt2, painter, w );

    // name on left
    painter->drawText( opt.rect.adjusted( checkRect.right() + PADDING, PADDING, -PADDING, -PADDING ), Qt::AlignLeft | Qt::AlignVCenter, acc->accountFriendlyName() );

    // remove, config, status on right
    const QRect pmRect( opt.rect.right() - PADDING - ICON_SIZE, topIcon, ICON_SIZE, ICON_SIZE );
    painter->drawPixmap( pmRect, TomahawkUtils::defaultPixmap( TomahawkUtils::ListRemove, TomahawkUtils::Original, pmRect.size() ) );
    m_cachedButtonRects[ index ] = pmRect;

    const QRect confRect( pmRect.left() - PADDING - CONFIG_WRENCH_SIZE, center - CONFIG_WRENCH_SIZE/2, CONFIG_WRENCH_SIZE, CONFIG_WRENCH_SIZE );

    QStyleOptionToolButton topt;
    topt.rect = confRect;
    topt.pos = confRect.topLeft();
    topt.font = opt.font;
    topt.icon = ImageRegistry::instance()->pixmap( RESPATH "images/configure.svg", QSize( CONFIG_WRENCH_SIZE - 8, CONFIG_WRENCH_SIZE - 8 ) );
    topt.iconSize = QSize( CONFIG_WRENCH_SIZE - 8, CONFIG_WRENCH_SIZE - 8 );
    topt.subControls = QStyle::SC_ToolButton;
    topt.activeSubControls = QStyle::SC_None;
    topt.features = QStyleOptionToolButton::None;
    bool pressed = ( m_configPressed == opt.index );
    topt.state = pressed ? QStyle::State_On : QStyle::State_Raised;
    if( opt.state & QStyle::State_MouseOver || pressed )
        topt.state |= QStyle::State_HasFocus;
    style->drawComplexControl( QStyle::CC_ToolButton, &topt, painter, w );
    m_cachedConfigRects[ index ] = confRect;

    QPixmap p;
    QString statusText;
    Account::ConnectionState state = acc->connectionState();
    const QRect connectIconRect( confRect.left() - PADDING - ICON_SIZE, topIcon, ICON_SIZE, ICON_SIZE );

    if ( state == Account::Connected )
    {
        p = TomahawkUtils::defaultPixmap( TomahawkUtils::SipPluginOnline, TomahawkUtils::Original, connectIconRect.size() );
        statusText = tr( "Online" );
    }
    else if ( state == Account::Connecting )
    {
        p = TomahawkUtils::defaultPixmap( TomahawkUtils::SipPluginOffline, TomahawkUtils::Original, connectIconRect.size() );
        statusText = tr( "Connecting..." );
    }
    else
    {
        p = TomahawkUtils::defaultPixmap( TomahawkUtils::SipPluginOffline, TomahawkUtils::Original, connectIconRect.size() );
        statusText = tr( "Offline" );
    }

    painter->drawPixmap( connectIconRect, p );

    int width = painter->fontMetrics().width( statusText );
    painter->drawText( QRect( connectIconRect.left() - PADDING - width, center - painter->fontMetrics().height()/2, width, painter->fontMetrics().height() ), statusText );

}

QSize
AccountFactoryWrapperDelegate::sizeHint(const QStyleOptionViewItem&, const QModelIndex&) const
{
    return QSize( 200, ACCOUNT_ROW_HEIGHT );
}


bool
AccountFactoryWrapperDelegate::editorEvent( QEvent* event, QAbstractItemModel*, const QStyleOptionViewItem&, const QModelIndex& index )
{
    if ( event->type() != QEvent::MouseButtonPress &&
        event->type() != QEvent::MouseButtonRelease &&
        event->type() != QEvent::MouseButtonDblClick &&
        event->type() != QEvent::MouseMove )
        return false;

    if ( event->type() == QEvent::MouseButtonPress )
    {
        // Show the config wrench as depressed on click
        QMouseEvent* me = static_cast< QMouseEvent* >( event );
        if ( me->button() == Qt::LeftButton && m_cachedConfigRects.contains( index ) && m_cachedConfigRects[ index ].contains( me->pos() ) )
        {
            m_configPressed = index;
            Account* acct = qobject_cast< Account* >( index.data( AccountFactoryWrapper::AccountRole ).value< QObject* >() );
            Q_ASSERT( acct ); // Should not be showing a config wrench if there is no account!

            emit openConfig( acct );

            return true;
        }
    } else if ( event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonDblClick )
    {
        QMouseEvent* me = static_cast< QMouseEvent* >( event );
        if ( m_configPressed.isValid() )
            emit update( m_configPressed );

        m_configPressed = QModelIndex();
        Account* acct = qobject_cast< Account* >( index.data( AccountFactoryWrapper::AccountRole ).value< QObject* >() );

        if ( m_cachedCheckRects.contains( index ) && m_cachedCheckRects[ index ].contains( me->pos() ) )
        {
            // Check box for this row
            // eat the double click events inside the check rect
            if( event->type() == QEvent::MouseButtonDblClick ) {
                return true;
            }

            Qt::CheckState curState = static_cast< Qt::CheckState >( index.data( Qt::CheckStateRole ).toInt() );
            Qt::CheckState newState = curState == Qt::Checked ? Qt::Unchecked : Qt::Checked;
            emit checkOrUncheck( index, acct, newState );
        }
        if ( m_cachedButtonRects.contains( index ) && m_cachedButtonRects[ index ].contains( me->pos() ) )
        {
            emit removeAccount( acct );

            return true;
        }
    }
    return false;
}

