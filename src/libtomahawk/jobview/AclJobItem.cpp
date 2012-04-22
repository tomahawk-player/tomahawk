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

#include "JobStatusModel.h"
#include "utils/TomahawkUtils.h"
#include "utils/TomahawkUtilsGui.h"
#include "libtomahawk/infosystem/InfoSystem.h"

#include <QPixmap>
#include <QPainter>
#include <QListView>
#include <QApplication>
#include <QMouseEvent>


#define ROW_HEIGHT 20
#define ICON_PADDING 1
#define PADDING 2


AclJobDelegate::AclJobDelegate( QObject* parent )
    : QStyledItemDelegate ( parent )
{
}


void
AclJobDelegate::paint( QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    //tDebug( LOGVERBOSE ) << Q_FUNC_INFO;
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );
    QFontMetrics fm( opt.font );

    opt.state &= ~QStyle::State_MouseOver;
    QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget );

    painter->setRenderHint( QPainter::Antialiasing );

    painter->fillRect( opt.rect, Qt::lightGray );

    QString mainText;
    AclJobItem* item = dynamic_cast< AclJobItem* >( index.data( JobStatusModel::JobDataRole ).value< JobStatusItem* >() );
    if ( !item )
        mainText = tr( "Error displaying ACL info" );
    else
        mainText = QString( tr( "Allow %1 to\nconnect and stream from you?" ) ).arg( item->username() );
    //tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Displaying text:" << mainText;
 
    const QString text = QString( tr( "Allow %1 to\nconnect and stream from you?" ) ).arg( item->username() );
    const QRect rRect( opt.rect.left() + PADDING, opt.rect.top() + 4*PADDING, opt.rect.width() - 2*PADDING, opt.rect.height() - 2*PADDING );
    painter->drawText( rRect, Qt::AlignHCenter, text );

    //tDebug( LOGVERBOSE ) << Q_FUNC_INFO << "Using rect " << rRect << ", opt rect is " << opt.rect;

    int totalwidth = opt.rect.width();
    int thirds = totalwidth/3;
    QRect btnRect;
    painter->setPen( Qt::white );
    
    QString btnText = tr( "Allow Streaming" );
    int btnWidth = fm.width( btnText ) + 2*PADDING;
    btnRect = QRect( opt.rect.left() + thirds - btnWidth/2, opt.rect.bottom() - fm.height() - 4*PADDING,  btnWidth + 2*PADDING, fm.height() + 2*PADDING );
    drawRoundedButton( painter, btnRect, btnRect.contains( m_savedHoverPos ) );
    painter->drawText( btnRect, Qt::AlignCenter, btnText );
    m_savedAcceptRect = btnRect;

    btnText = tr( "Deny Access" );
    btnWidth = fm.width( btnText ) + 2*PADDING;
    btnRect = QRect( opt.rect.right() - thirds - btnWidth/2, opt.rect.bottom() - fm.height() - 4*PADDING,  btnWidth + 2*PADDING, fm.height() + 2*PADDING );
    drawRoundedButton( painter, btnRect, btnRect.contains( m_savedHoverPos ) );
    painter->drawText( btnRect, Qt::AlignCenter, btnText );
    m_savedDenyRect = btnRect;
}

QSize
AclJobDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    QSize size( QStyledItemDelegate::sizeHint ( option, index ).width(), ROW_HEIGHT * 3 );
    return size;
}


void
AclJobDelegate::drawRoundedButton( QPainter* painter, const QRect& btnRect, bool red ) const
{
    if ( !red )
        TomahawkUtils::drawRoundedButton( painter, btnRect, QColor(54, 127, 211), QColor(43, 104, 182), QColor(34, 85, 159), QColor(35, 79, 147) );
    else
        TomahawkUtils::drawRoundedButton( painter, btnRect, QColor(206, 63, 63), QColor(170, 52, 52), QColor(150, 50, 50), QColor(130, 40, 40) );
}


bool
AclJobDelegate::editorEvent( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
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



AclJobItem::AclJobItem( ACLRegistry::User user, const QString &username )
    : m_delegate( 0 )
    , m_user( user )
    , m_username( username )
{
}


AclJobItem::~AclJobItem()
{
}


void
AclJobItem::createDelegate( QObject* parent )
{
    if ( m_delegate )
        return;

    m_delegate = new AclJobDelegate( parent );

    Tomahawk::InfoSystem::InfoPushData pushData( "AclJobItem", Tomahawk::InfoSystem::InfoNotifyUser, tr( "Tomahawk needs you to decide whether %1 is allowed to connect." ).arg( m_username ), Tomahawk::InfoSystem::PushNoFlag );
    Tomahawk::InfoSystem::InfoSystem::instance()->pushInfo( pushData );
}


void
AclJobDelegate::emitSizeHintChanged( const QModelIndex& index )
{
    emit sizeHintChanged( index );
}


void
AclJobItem::aclResult( ACLRegistry::ACL result )
{
    m_user.acl = result;
    emit userDecision( m_user );
    done();
}


void
AclJobItem::done()
{
    emit finished();
}

