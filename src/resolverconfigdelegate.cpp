/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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


#include "resolverconfigdelegate.h"

#include "resolversmodel.h"
#include "tomahawk/tomahawkapp.h"

#include <QApplication>
#include <QPainter>
#include <QMouseEvent>

#define PADDING 4

ResolverConfigDelegate::ResolverConfigDelegate( QObject* parent )
    : QStyledItemDelegate( parent )
    , m_configPressed( false )
{

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


    QFontMetrics bfm( name );
    QFontMetrics sfm( path );

    // draw the background
    const QWidget* w = opt.widget;
    QStyle* style = w ? w->style() : QApplication::style();
    style->drawPrimitive( QStyle::PE_PanelItemViewItem, &opt, painter, w );

    int rightSplit = itemRect.width();
    int rectW = opt.rect.height() - 4 * PADDING;
    QRect confRect = QRect( rightSplit - rectW - 2 * PADDING, 2 * PADDING + top, rectW, rectW );
    // if the resolver has a config widget, paint it first (right-aligned)
    if( index.data( ResolversModel::HasConfig ).toBool() ) {
        // draw it the same size as the check belox
        QStyleOptionToolButton topt;
        topt.font = opt.font;
        topt.icon = QIcon( RESPATH "images/configure.png" );
        topt.iconSize = QSize( 16, 16 );
        topt.rect = confRect;
        topt.subControls = QStyle::SC_ToolButton;
        topt.activeSubControls = QStyle::SC_None;
        topt.features = QStyleOptionToolButton::None;
        topt.pos = confRect.topLeft();
        topt.state = m_configPressed ? QStyle::State_On : QStyle::State_Raised;
        if( opt.state & QStyle::State_MouseOver || m_configPressed )
            topt.state |= QStyle::State_HasFocus;
        style->drawComplexControl( QStyle::CC_ToolButton, &topt, painter, w );
    }

    // draw check
    confRect.moveTo( 2 * PADDING, 2 * PADDING + top );
    opt.rect = confRect;
    opt.checkState == Qt::Checked ? opt.state |= QStyle::State_On : opt.state |= QStyle::State_Off;
    style->drawPrimitive( QStyle::PE_IndicatorViewItemCheck, &opt, painter, w );
    itemRect.setX( opt.rect.topRight().x() + PADDING );

    QString nameStr = bfm.elidedText( index.data( ResolversModel::ResolverName ).toString(),Qt::ElideRight, rightSplit );
    painter->save();
    painter->setFont( name );
    QRect textRect = itemRect.adjusted( PADDING, PADDING, -PADDING, -PADDING );
    textRect.setBottom( itemRect.height() / 2 + top  );
    painter->drawText( textRect, nameStr );
    painter->restore();

    QString pathStr = sfm.elidedText( index.data( ResolversModel::ResolverPath ).toString(),Qt::ElideMiddle, rightSplit );
    painter->save();
    painter->setFont( path );
    painter->setBrush( Qt::gray );
    textRect.moveTop(  itemRect.height() / 2 + top );
    painter->drawText( textRect, pathStr );
    painter->restore();

}

QSize
ResolverConfigDelegate::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
{
    int width = QStyledItemDelegate::sizeHint( option, index ).width();

    QStyleOptionViewItemV4 opt = option;
    initStyleOption( &opt, index );


    QFont name = opt.font;
    name.setPointSize( name.pointSize() + 2 );
    name.setBold( true );

    QFont path = opt.font;
    path.setItalic( true );
    path.setPointSize( path.pointSize() - 1 );


    QFontMetrics bfm( name );
    QFontMetrics sfm( path );
    return QSize( width, 3 * PADDING + bfm.height() + sfm.height() );
}

bool
ResolverConfigDelegate::editorEvent(QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index)
{
//     qDebug() << "EDITOR EVENT!" << ( event->type() == QEvent::MouseButtonRelease );

    QStyleOptionViewItemV4 viewOpt( option );
    initStyleOption( &viewOpt, index );
    const QWidget* w = viewOpt.widget;
    QStyle* style = w ? w->style() : QApplication::style();
    int top = viewOpt.rect.top();

    if( event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonDblClick ) {
        m_configPressed = false;

        int rectW = option.rect.height() - 4 * PADDING;
        QRect checkRect = QRect( 2 * PADDING, 2 * PADDING + top, rectW, rectW );
        QMouseEvent* me = static_cast< QMouseEvent* >( event );
        if( me->button() != Qt::LeftButton || !checkRect.contains( me->pos() ) )
            return false;

        // eat the double click events inside the check rect
        if( event->type() == QEvent::MouseButtonDblClick ) {
            return true;
        }

        Qt::CheckState curState = static_cast< Qt::CheckState >( index.data( Qt::CheckStateRole ).toInt() );
        Qt::CheckState newState = curState == Qt::Checked ? Qt::Unchecked : Qt::Checked;
        return model->setData( index, newState, Qt::CheckStateRole );

    } else if( event->type() == QEvent::MouseButtonPress ) {
        int rightSplit = viewOpt.rect.width();
        int rectW = viewOpt.rect.height() - 4 * PADDING;
        QRect confRect = QRect( rightSplit - rectW - 2 * PADDING, 2 * PADDING + top, rectW, rectW );

        QMouseEvent* me = static_cast< QMouseEvent* >( event );
        if( me->button() == Qt::LeftButton && confRect.contains( me->pos() ) ) {
            m_configPressed = true;

            emit openConfig( index.data( ResolversModel::ResolverPath ).toString() );
            return true;
        }
    }

    return QStyledItemDelegate::editorEvent( event, model, option, index );
}
