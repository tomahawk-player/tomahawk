/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
 *   Copyright 2012 Teo Mrnjavac <teo@kde.org>
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

#include "AccountsPopupWidget.h"

#include "utils/TomahawkUtilsGui.h"

#include <QDebug>
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>

AccountsPopupWidget::AccountsPopupWidget( QWidget* parent )
    : QWidget( parent )
    , m_widget( 0 )
    , m_arrowOffset( 0 )
{
    setWindowFlags( Qt::Popup | Qt::FramelessWindowHint );

    //Uncomment this if using a debugger:
    //setWindowFlags( Qt::Window );

    setAutoFillBackground( false );
    setAttribute( Qt::WA_TranslucentBackground, true );
    setAttribute( Qt::WA_NoSystemBackground, true );

    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );

    m_layout = new QVBoxLayout( this );
    setLayout( m_layout );

    setContentsMargins( contentsMargins().left() + 2, contentsMargins().top() + 2 + 6 /*arrowHeight*/ ,
                        contentsMargins().right(), contentsMargins().bottom() );

#ifdef Q_OS_MAC
    setContentsMargins( 0, 0, 0, 0 );
    layout()->setContentsMargins( 0, 0, 0, 0 );
#endif
}


void
AccountsPopupWidget::setWidget( QWidget* widget )
{
    m_widget = widget;
    m_layout->addWidget( m_widget );
    m_widget->setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );
}


void
AccountsPopupWidget::anchorAt( const QPoint &p )
{
    QPoint myTopRight( p.x() - sizeHint().width(), p.y() - 2 ); //we go 2px up to point inside the button

    move( myTopRight );
    if( isVisible() )
        repaint();
}


void
AccountsPopupWidget::setArrowOffset( int arrowOffset )
{
    arrowOffset -= 2; //a pinch of magic dust to handle the internal margin
    if ( arrowOffset != m_arrowOffset )
    {
        m_arrowOffset = arrowOffset;
        if ( isVisible() )
            repaint();
    }
}


void
AccountsPopupWidget::paintEvent( QPaintEvent* )
{
    // Constants for painting
    const int cornerRadius = TomahawkUtils::POPUP_ROUNDING_RADIUS;   //the rounding radius of the widget
    const int arrowHeight = 6;

    //m_arrowOffset is the distance between the far right boundary and the x value of the arrow head.
    //It is provided by the tool button, and is expected to be the middle of the button.
    //With this, we make sure that whatever happens, it will be bigger than the rounding radius plus
    //half the arrow, so that the shape of the rounded rect won't be affected.
    m_arrowOffset = qMax( m_arrowOffset, cornerRadius + arrowHeight ); //at least 12!

    const QRect brect = rect().adjusted( 2, arrowHeight + 2, -2, -2 );

    QPainterPath outline;
    outline.addRoundedRect( brect.left(), brect.top(), brect.width(), brect.height(), cornerRadius, cornerRadius );
    outline.moveTo( rect().right() - m_arrowOffset + arrowHeight, brect.top() );
    outline.lineTo( rect().right() - m_arrowOffset, 2 );
    outline.lineTo( rect().right() - m_arrowOffset - arrowHeight, brect.top() );

    TomahawkUtils::drawCompositedPopup( this,
                                        outline,
                                        TomahawkUtils::Colors::BORDER_LINE,
                                        TomahawkUtils::Colors::POPUP_BACKGROUND,
                                        TomahawkUtils::POPUP_OPACITY );
}


void
AccountsPopupWidget::focusOutEvent( QFocusEvent* )
{
    hide();
}


void
AccountsPopupWidget::hideEvent( QHideEvent* )
{
    emit hidden();
}

