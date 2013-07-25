/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012-2013, Teo Mrnjavac <teo@kde.org>
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

#include "BasicHeader.h"

#include "ElidedLabel.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QLabel>
#include <QPixmap>
#include <QPaintEvent>
#include <QPainter>
#include <QBoxLayout>
#include <QGraphicsDropShadowEffect>

using namespace Tomahawk;


BasicHeader::BasicHeader( QWidget* parent )
    : QWidget( parent )
{
    QLayout* l = new QVBoxLayout;
    TomahawkUtils::unmarginLayout( l );
    setLayout( l );

    m_mainLayout = new QHBoxLayout;

    m_imageLabel = new QLabel( this );
    m_imageLabel->setMargin( 0 );
    m_imageLabel->setFixedSize( TomahawkUtils::defaultIconSize().width() * 3,
                                TomahawkUtils::defaultIconSize().height() * 3 );

    m_mainLayout->addWidget( m_imageLabel );
    m_mainLayout->addSpacing( TomahawkUtils::defaultIconSize().width() / 4 );

    m_verticalLayout = new QVBoxLayout;
    m_mainLayout->addLayout( m_verticalLayout );

    m_captionLabel = new ElidedLabel( this );
    m_descriptionLabel = new ElidedLabel( this );
    m_verticalLayout->addWidget( m_captionLabel );
    m_verticalLayout->addWidget( m_descriptionLabel );
    m_verticalLayout->addStretch();

    m_mainLayout->addSpacing( TomahawkUtils::defaultIconSize().width() / 4 );
    m_mainLayout->setStretchFactor( m_verticalLayout, 2 );

    QPalette pal = palette();
    pal.setColor( QPalette::Foreground, TomahawkStyle::HEADER_TEXT );
    pal.setBrush( backgroundRole(), TomahawkStyle::HEADER_BACKGROUND );

    m_captionLabel->setPalette( pal );
    m_descriptionLabel->setPalette( pal );

    QFont font = m_captionLabel->font();
    
    int captionFontSize = TomahawkUtils::defaultFontSize() + 6;
    font.setPointSize( captionFontSize );
    font.setBold( true );
    font.setFamily( "Titillium Web" );
    
    m_captionLabel->setFont( font );
    m_captionLabel->setElideMode( Qt::ElideRight );
    m_captionLabel->setAlignment( Qt::AlignTop | Qt::AlignLeft );
    m_captionLabel->setMargin( 2 );
    m_captionLabel->setMinimumHeight( QFontMetrics( font ).height() + 2 * m_captionLabel->margin() );

    int descriptionFontSize = TomahawkUtils::defaultFontSize() + 2;
    font.setPointSize( descriptionFontSize );
    font.setBold( false );
    m_descriptionLabel->setFont( font );
    m_descriptionLabel->setElideMode( Qt::ElideRight );
    m_descriptionLabel->setAlignment( Qt::AlignTop | Qt::AlignLeft );
    m_descriptionLabel->setMargin( 2 );
    m_descriptionLabel->setMinimumHeight( QFontMetrics( font ).height() + 2 * m_descriptionLabel->margin() );
    m_descriptionLabel->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Preferred );

/*    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
    effect->setBlurRadius( 4 );
    effect->setXOffset( 0 );
    effect->setYOffset( 0 );
    effect->setColor( Qt::white );
    m_captionLabel->setGraphicsEffect( effect );*/
//    m_descriptionLabel->setGraphicsEffect( effect );

    QFrame* lineAbove = new QFrame( this );
    lineAbove->setStyleSheet( QString( "QFrame { border: 1px solid %1; }" ).arg( TomahawkStyle::HEADER_BACKGROUND.name() ) );
    lineAbove->setFrameShape( QFrame::HLine );
    lineAbove->setMaximumHeight( 1 );
    QFrame* lineBelow = new QFrame( this );
    lineBelow->setStyleSheet( QString( "QFrame { border: 1px solid black; }" ) );
    lineBelow->setFrameShape( QFrame::HLine );
    lineBelow->setMaximumHeight( 1 );

    l->addItem( m_mainLayout );
    l->addWidget( lineAbove );
    l->addWidget( lineBelow );

    TomahawkUtils::unmarginLayout( m_mainLayout );

    // on 72dpi, 1px = 1pt
    // margins that should be around 8 4 8 4 on ~100dpi
    int leftRightMargin = TomahawkUtils::defaultFontHeight() / 3;
    int topBottomMargin = TomahawkUtils::defaultFontHeight() / 6;

    m_mainLayout->setContentsMargins( leftRightMargin, topBottomMargin,
                                      leftRightMargin, topBottomMargin );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    // top-margin + header + layout spacing + description + bottom-margin
    setFixedHeight( qMax( topBottomMargin + m_captionLabel->height() + TomahawkUtils::defaultIconSize().height() / 4 + m_descriptionLabel->height() + topBottomMargin,
                          topBottomMargin + m_imageLabel->height() + topBottomMargin ) );

    setAutoFillBackground( true );
    setPalette( pal );
}


BasicHeader::~BasicHeader()
{
}


void
BasicHeader::setCaption( const QString& s )
{
    m_captionLabel->setText( s );
}


void
BasicHeader::setDescription( const QString& s )
{
    m_descriptionLabel->setText( s );
}


void
BasicHeader::setPixmap( const QPixmap& p )
{
    m_imageLabel->setPixmap( p.scaledToHeight( m_imageLabel->height(), Qt::SmoothTransformation ) );
}


void
BasicHeader::paintEvent( QPaintEvent* event )
{
    QWidget::paintEvent( event );

/*    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing );

    QLinearGradient gradient( QPoint( 0, 0 ), QPoint( 0, 1 ) );
    gradient.setCoordinateMode( QGradient::ObjectBoundingMode );
    gradient.setColorAt( 0.0, TomahawkStyle::HEADER_LOWER );
    gradient.setColorAt( 1.0, TomahawkStyle::HEADER_UPPER );

    painter.setBrush( gradient );
    painter.fillRect( rect(), gradient );*/
}
