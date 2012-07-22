/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2012 Leo Franchi <lfranchi@kde.org>
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

#include "SourceTreePopupDialog.h"

#include "sourcetree/SourceTreeView.h"

#include <QPaintEvent>
#include <QPainter>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTimer>

SourceTreePopupDialog::SourceTreePopupDialog( SourceTreeView* parent )
    : QWidget( 0 )
    , m_result( false )
    , m_label( 0 )
    , m_buttons( 0 )
{
    setWindowFlags( Qt::FramelessWindowHint );
    setWindowFlags( Qt::Popup );

    setAutoFillBackground( false );
    setAttribute( Qt::WA_TranslucentBackground, true );

    setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

    m_label = new QLabel( this );
    m_buttons = new QDialogButtonBox( QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this );
    m_buttons->button( QDialogButtonBox::Ok )->setIcon( QIcon() );
    m_buttons->button( QDialogButtonBox::Cancel )->setIcon( QIcon() );

    connect( m_buttons, SIGNAL( accepted() ), this, SLOT( onAccepted() ) );
    connect( m_buttons, SIGNAL( rejected() ), this, SLOT( onRejected() ) );

    setLayout( new QVBoxLayout );

    layout()->addWidget( m_label );
    layout()->addWidget( m_buttons );
/*
    m_buttons->button( QDialogButtonBox::Ok )->setStyleSheet(
                       "QPushButton { \
                            background-color: #F15C5E; \
                            border-style: solid; \
                            border-width: 1px; \
                            border-radius: 10px; \
                            border-color: #B64547; \
                            padding: 2px; \
                        } \
                        QPushButton:pressed { \
                            border-style: solid; \
                            border-width: 1px; \
                            border-radius: 10px; \
                            border-color: #B64547; \
                            background-color: #D35052; \
                            border-style: flat; \
                        }" );*/
}


void
SourceTreePopupDialog::setMainText( const QString& text )
{
    m_label->setText( text );
}


void
SourceTreePopupDialog::setOkButtonText( const QString& text )
{
    if ( m_buttons && m_buttons->button( QDialogButtonBox::Ok ) )
        m_buttons->button( QDialogButtonBox::Ok )->setText( text );
}


void
SourceTreePopupDialog::paintEvent( QPaintEvent* event )
{
    // Constants for painting
    const int leftTriangleWidth = 20;
    const int cornerRounding = 8;
    const int leftEdgeOffset = offset() - 6;
    const QRect brect = rect().adjusted( 2, 3, -2, -3 );

    QPainterPath outline;
    // Left triangle top branch
    outline.moveTo( brect.left(), brect.height() / 2 );
    outline.lineTo( leftEdgeOffset, brect.height() / 2 - leftTriangleWidth / 2 );

    // main outline
    outline.lineTo( leftEdgeOffset, cornerRounding );
    outline.quadTo( QPoint( leftEdgeOffset, brect.top() ), QPoint( leftEdgeOffset + cornerRounding, brect.top() ) );
    outline.lineTo( brect.width() - cornerRounding, brect.top() );
    outline.quadTo( QPoint( brect.width(), brect.top() ), QPoint( brect.width(), cornerRounding ) );
    outline.lineTo( brect.width(), brect.height() - cornerRounding );
    outline.quadTo( brect.bottomRight(), QPoint( brect.right() - cornerRounding, brect.height() ) );
    outline.lineTo( leftEdgeOffset + cornerRounding, brect.height() );
    outline.quadTo( QPoint( leftEdgeOffset, brect.height() ), QPoint( leftEdgeOffset, brect.height() - cornerRounding ) );

    // Left triangle bottom branch
    outline.lineTo( leftEdgeOffset, brect.height() / 2 + leftTriangleWidth / 2 );
    outline.lineTo( brect.left(), brect.height() / 2 );

    QPainter p( this );
    p.setRenderHint( QPainter::Antialiasing );

    QPen pen( QColor( "#3F4247" ) );
    pen.setWidth( 2 );
    p.setPen( pen );
    p.drawPath( outline );

    p.fillPath( outline, QColor( "#D6E3F1" ) );
}


void
SourceTreePopupDialog::focusOutEvent( QFocusEvent* )
{
    hide();
}


void
SourceTreePopupDialog::showEvent( QShowEvent* )
{
    m_result = false;
}


void
SourceTreePopupDialog::onAccepted()
{
    hide();
    m_result = true;
    emit result( m_result );
}


void
SourceTreePopupDialog::onRejected()
{
    hide();
    m_result = false;
    emit result( m_result );
}
