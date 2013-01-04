/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2012, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "AclJobItem.h"

#include <QPixmap>
#include <QPainter>
#include <QListView>
#include <QApplication>
#include <QMouseEvent>

#include "JobStatusModel.h"
#include "infosystem/InfoSystem.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#define ICON_PADDING 1
#define PADDING 2


ACLJobDelegate::ACLJobDelegate( QObject* parent )
    : QStyledItemDelegate ( parent )
{
    tLog() << Q_FUNC_INFO;
}


ACLJobDelegate::~ACLJobDelegate()
{
    tLog() << Q_FUNC_INFO;
}


void
ACLJobDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    ACLJobItem* item = dynamic_cast< ACLJobItem* >( index.data( JobStatusModel::JobDataRole ).value< JobStatusItem* >() );
    if ( !item )
        return;
    //tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );
    QFontMetrics fm( opt.font );

    opt.state &= ~QStyle::State_MouseOver;
    QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget );

    painter->setRenderHint( QPainter::Antialiasing );
    painter->fillRect( opt.rect, Qt::lightGray );

    QString mainText = QString( tr( "Allow %1 to\nconnect and stream from you?" ) ).arg( item->username() );
    //tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Displaying text:" << mainText;
 
    const QRect rRect( opt.rect.left() + PADDING, opt.rect.top() + 4*PADDING, opt.rect.width() - 2*PADDING, opt.rect.height() - 2*PADDING );
    painter->drawText( rRect, Qt::AlignHCenter, mainText );

    //tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Using rect " << rRect << ", opt rect is " << opt.rect;

    int totalwidth = opt.rect.width();
    int thirds = totalwidth/3;
    QRect allowBtnRect;
    QRect denyBtnRect;
    painter->setPen( Qt::white );

    int minPixels = 20;
    
    QString allowBtnText = tr( "Allow Streaming" );
    int allowBtnWidth = fm.width( allowBtnText ) + 2 * PADDING;
    allowBtnRect = QRect( opt.rect.left() + thirds - allowBtnWidth / 2, opt.rect.bottom() - fm.height() - 4 * PADDING,  allowBtnWidth + 2 * PADDING, fm.height() + 2 * PADDING );
    QString denyBtnText = tr( "Deny Access" );
    int denyBtnWidth = fm.width( denyBtnText ) + 2 * PADDING;
    denyBtnRect = QRect( opt.rect.right() - thirds - denyBtnWidth / 2, opt.rect.bottom() - fm.height() - 4 * PADDING,  denyBtnWidth + 2 * PADDING, fm.height() + 2 * PADDING );

    if ( allowBtnRect.right() >= denyBtnRect.left() )
    {
        allowBtnRect.moveLeft( allowBtnRect.left() - minPixels / 2 );
        denyBtnRect.moveLeft( denyBtnRect.left() + minPixels / 2 );
    }

    drawRoundedButton( painter, allowBtnRect, allowBtnRect.contains( m_savedHoverPos ) );
    painter->drawText( allowBtnRect, Qt::AlignCenter, allowBtnText );
    m_savedAcceptRect = allowBtnRect;
    drawRoundedButton( painter, denyBtnRect, denyBtnRect.contains( m_savedHoverPos ) );
    painter->drawText( denyBtnRect, Qt::AlignCenter, denyBtnText );
    m_savedDenyRect = denyBtnRect;
}


QSize
ACLJobDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size( QStyledItemDelegate::sizeHint( option, index ).width(), ( TomahawkUtils::defaultFontHeight() + 6 ) * 3.5 );
    return size;
}


void
ACLJobDelegate::drawRoundedButton( QPainter* painter, const QRect& btnRect, bool red ) const
{
    if ( !red )
        TomahawkUtils::drawRoundedButton( painter, btnRect, QColor( 54, 127, 211 ), QColor( 43, 104, 182 ), QColor( 34, 85, 159 ), QColor( 35, 79, 147 ) );
    else
        TomahawkUtils::drawRoundedButton( painter, btnRect, QColor( 206, 63, 63 ), QColor( 170, 52, 52 ), QColor( 150, 50, 50 ), QColor( 130, 40, 40 ) );
}


bool
ACLJobDelegate::editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
    Q_UNUSED( option )
    Q_UNUSED( model )
    //tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    if ( event->type() != QEvent::MouseButtonPress &&
         event->type() != QEvent::MouseButtonRelease &&
         event->type() != QEvent::MouseButtonDblClick &&
         event->type() != QEvent::MouseMove )
        return false;

    if ( event->type() == QEvent::MouseMove )
    {
        QMouseEvent* me = static_cast< QMouseEvent* >( event );
        m_savedHoverPos = me->pos();
        //tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Setting position to " << m_savedHoverPos;
        emit update( index );
        return true;
    }

    if ( event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonDblClick )
    {
        QMouseEvent* me = static_cast< QMouseEvent* >( event );
        if ( m_savedAcceptRect.contains( me->pos() ) )
            emit aclResult( ACLRegistry::Stream );
        else if ( m_savedDenyRect.contains( me->pos() ) )
            emit aclResult( ACLRegistry::Deny );
        return true;
    }

    return false;
}



ACLJobItem::ACLJobItem( ACLRegistry::User user, const QString &username )
    : m_delegate( 0 )
    , m_user( user )
    , m_username( username )
{
    tLog() << Q_FUNC_INFO;
}


ACLJobItem::~ACLJobItem()
{
    tLog() << Q_FUNC_INFO;
}


void
ACLJobItem::createDelegate( QObject* parent )
{
    tLog() << Q_FUNC_INFO;
    
    if ( m_delegate )
        return;

    m_delegate = new ACLJobDelegate( parent );

    Tomahawk::InfoSystem::InfoPushData pushData( "ACLJobItem", Tomahawk::InfoSystem::InfoNotifyUser, tr( "Tomahawk needs you to decide whether %1 is allowed to connect." ).arg( m_username ), Tomahawk::InfoSystem::PushNoFlag );
    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( pushData );
}


void
ACLJobDelegate::emitSizeHintChanged( const QModelIndex& index )
{
    emit sizeHintChanged( index );
}


void
ACLJobItem::aclResult( ACLRegistry::ACL result )
{
    tLog() << Q_FUNC_INFO;
    m_user.acl = result;
    emit userDecision( m_user );
    emit finished();
}

