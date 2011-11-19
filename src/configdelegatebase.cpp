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


#include "configdelegatebase.h"

#include <QPainter>
#include <QApplication>
#include <QMouseEvent>

#include "utils/tomahawkutils.h"
#include "utils/logger.h"

ConfigDelegateBase::ConfigDelegateBase ( QObject* parent )
    : QStyledItemDelegate ( parent )
{

}


QSize
ConfigDelegateBase::sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
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
    return QSize( width, 2 * PADDING + bfm.height() + sfm.height() );
}

void
ConfigDelegateBase::drawCheckBox( QStyleOptionViewItemV4& opt, QPainter* p, const QWidget* w ) const
{
    QStyle* style = w ? w->style() : QApplication::style();
    opt.checkState == Qt::Checked ? opt.state |= QStyle::State_On : opt.state |= QStyle::State_Off;
    style->drawPrimitive( QStyle::PE_IndicatorViewItemCheck, &opt, p, w );
}


void
ConfigDelegateBase::drawConfigWrench ( QPainter* painter, QStyleOptionViewItemV4& opt, QStyleOptionToolButton& topt ) const
{
    const QWidget* w = opt.widget;
    QStyle* style = w ? w->style() : QApplication::style();

    // draw it the same size as the check belox
    topt.font = opt.font;
    topt.icon = QIcon( RESPATH "images/configure.png" );
    topt.iconSize = QSize( 16, 16 );
    topt.subControls = QStyle::SC_ToolButton;
    topt.activeSubControls = QStyle::SC_None;
    topt.features = QStyleOptionToolButton::None;
    bool pressed = ( m_configPressed == opt.index );
    topt.state = pressed ? QStyle::State_On : QStyle::State_Raised;
    if( opt.state & QStyle::State_MouseOver || pressed )
        topt.state |= QStyle::State_HasFocus;
    style->drawComplexControl( QStyle::CC_ToolButton, &topt, painter, w );
}

bool
ConfigDelegateBase::editorEvent ( QEvent* event, QAbstractItemModel* model, const QStyleOptionViewItem& option, const QModelIndex& index )
{
    QStyleOptionViewItemV4 viewOpt( option );
    initStyleOption( &viewOpt, index );

    if( event->type() == QEvent::MouseButtonRelease || event->type() == QEvent::MouseButtonDblClick ) {
        m_configPressed = QModelIndex();

        QMouseEvent* me = static_cast< QMouseEvent* >( event );
        if( me->button() != Qt::LeftButton || !checkRectForIndex( option, index ).contains( me->pos() ) )
            return false;

        // eat the double click events inside the check rect
        if( event->type() == QEvent::MouseButtonDblClick ) {
            return true;
        }

        Qt::CheckState curState = static_cast< Qt::CheckState >( index.data( Qt::CheckStateRole ).toInt() );
        Qt::CheckState newState = curState == Qt::Checked ? Qt::Unchecked : Qt::Checked;
        return model->setData( index, newState, Qt::CheckStateRole );

    } else if( event->type() == QEvent::MouseButtonPress ) {
        QMouseEvent* me = static_cast< QMouseEvent* >( event );
        if( me->button() == Qt::LeftButton && configRectForIndex( option, index ).contains( me->pos() ) ) {
            m_configPressed = index;

            emit configPressed( index );
            return true;
        }
    }

    return QStyledItemDelegate::editorEvent( event, model, option, index );
}
