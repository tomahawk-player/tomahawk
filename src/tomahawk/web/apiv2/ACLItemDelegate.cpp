/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013,      Uwe L. Korn <uwelk@xhochy.com>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ACLItemDelegate.h"

#include "jobview/JobStatusModel.h"
#include "utils/TomahawkUtils.h"
#include "utils/TomahawkUtilsGui.h"
#include "ACLJobStatusItem.h"

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>

namespace Tomahawk {
namespace APIv2 {

#define PADDING 2

ACLItemDelegate::ACLItemDelegate( QObject *parent )
    : QStyledItemDelegate( parent )
{

}


ACLItemDelegate::~ACLItemDelegate()
{

}


void ACLItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    ACLJobStatusItem* item = dynamic_cast< ACLJobStatusItem* >( index.data( JobStatusModel::JobDataRole ).value< JobStatusItem* >() );
    if ( !item )
        return;
    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );
    QFontMetrics fm( opt.font );

    opt.state &= ~QStyle::State_MouseOver;
    QApplication::style()->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, painter, opt.widget );

    painter->setRenderHint( QPainter::Antialiasing );
    painter->fillRect( opt.rect, Qt::lightGray );

    QString mainText = QString( tr( "%An instance of\n%1\nwants to control this player:" ) ).arg( item->user()->clientName() );

    const QRect rRect( opt.rect.left() + PADDING, opt.rect.top() + 4*PADDING, opt.rect.width() - 2*PADDING, opt.rect.height() - 2*PADDING );
    painter->drawText( rRect, Qt::AlignHCenter, mainText );

    int totalwidth = opt.rect.width();
    int thirds = totalwidth/3;
    QRect allowBtnRect;
    painter->setPen( Qt::white );

    QString allowBtnText = tr( "See Details & Decide" );
    int allowBtnWidth = fm.width( allowBtnText ) + 2 * PADDING;
    allowBtnRect = QRect( opt.rect.left() + thirds - allowBtnWidth / 2, opt.rect.bottom() - fm.height() - 4 * PADDING,  allowBtnWidth + 2 * PADDING, fm.height() + 2 * PADDING );


    drawRoundedButton( painter, allowBtnRect, allowBtnRect.contains( m_savedHoverPos ) );
    painter->drawText( allowBtnRect, Qt::AlignCenter, allowBtnText );
    m_savedAcceptRect = allowBtnRect;
}


QSize
ACLItemDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QSize size( QStyledItemDelegate::sizeHint( option, index ).width(), ( TomahawkUtils::defaultFontHeight() + 6 ) * 4.5 );
    return size;
}


void
ACLItemDelegate::emitSizeHintChanged( const QModelIndex& index )
{
    emit sizeHintChanged( index );
}

void
ACLItemDelegate::drawRoundedButton( QPainter *painter, const QRect& btnRect, bool red ) const
{
    //FIXME const colors
    if ( !red )
        TomahawkUtils::drawRoundedButton( painter, btnRect, QColor( 54, 127, 211 ), QColor( 43, 104, 182 ), QColor( 34, 85, 159 ), QColor( 35, 79, 147 ) );
    else
        TomahawkUtils::drawRoundedButton( painter, btnRect, QColor( 206, 63, 63 ), QColor( 170, 52, 52 ), QColor( 150, 50, 50 ), QColor( 130, 40, 40 ) );
}

bool
ACLItemDelegate::editorEvent( QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem& option, const QModelIndex& index )
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

    /* TODO
    if ( event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonDblClick )
    {
        QMouseEvent* me = static_cast< QMouseEvent* >( event );
        if ( m_savedAcceptRect.contains( me->pos() ) )
            emit aclResult( Tomahawk::ACLStatus::Stream );
        else if ( m_savedDenyRect.contains( me->pos() ) )
            emit aclResult( Tomahawk::ACLStatus::Deny );
        return true;
    }
    */

    return false;
}


} // namespace APIv2
} // namespace Tomahawk
