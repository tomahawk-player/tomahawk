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

#include <QDebug>
#include <QPainter>
#include <QPaintEvent>
#include <QVBoxLayout>

#ifdef QT_MAC_USE_COCOA
#include "widgets/SourceTreePopupDialog_mac.h"
#endif

AccountsPopupWidget::AccountsPopupWidget( QWidget* parent )
    : QWidget( parent )
    , m_widget( 0 )
{
    setWindowFlags( Qt::FramelessWindowHint );
    setWindowFlags( Qt::Popup );
    //Uncomment this if using a debugger:
    //setWindowFlags( Qt::Window );

    setAutoFillBackground( false );
    setAttribute( Qt::WA_TranslucentBackground, true );

    setSizePolicy( QSizePolicy::Preferred, QSizePolicy::Minimum );

    m_layout = new QVBoxLayout( this );
    setLayout( m_layout );

    setContentsMargins( contentsMargins().left() + 2, contentsMargins().top() + 2 ,
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
#ifdef Q_OS_WIN
    // We do this because Windows sticks the toolbutton really close to the
    // right side border of the window
    QPoint myTopRight( p.x() - sizeHint().width(), p.y() );
#else
    QPoint myTopRight( p.x() - sizeHint().width() + 8, p.y() );
#endif
    move( myTopRight );
    if( isVisible() )
        repaint();
}

void AccountsPopupWidget::paintEvent( QPaintEvent* )
{
    // Constants for painting
    const int cornerRadius = 8;   //the rounding radius of the widget

    const QRect brect = rect().adjusted( 2, 2, -2, -2 );

    QPainterPath outline;
    outline.addRoundedRect( 3, 3, brect.width(), brect.height(), cornerRadius, cornerRadius );

    QPainter p( this );

    p.setRenderHint( QPainter::Antialiasing );
    p.setBackgroundMode( Qt::TransparentMode );

    QPen pen( QColor( 0x8c, 0x8c, 0x8c ) );
    pen.setWidth( 2 );
    p.setPen( pen );
    p.drawPath( outline );

    p.setOpacity( 0.96 );
    p.fillPath( outline, QColor( "#FFFFFF" ) );

#ifdef QT_MAC_USE_COCOA
    // Work around bug in Qt/Mac Cocoa where opening subsequent popups
    // would incorrectly calculate the background due to it not being
    // invalidated.
    SourceTreePopupHelper::clearBackground( this );
#endif
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
