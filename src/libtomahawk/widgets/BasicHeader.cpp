/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012,      Teo Mrnjavac <teo@kde.org>
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

#include "utils/TomahawkUtilsGui.h"
#include "ElidedLabel.h"

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
    m_mainLayout = new QHBoxLayout;
    setLayout( m_mainLayout );

    m_imageLabel = new QLabel( this );
    m_imageLabel->setFixedSize( 64, 64 );
    m_mainLayout->addWidget( m_imageLabel );
    m_mainLayout->addSpacing( 16 );

    m_verticalLayout = new QVBoxLayout;
    m_mainLayout->addLayout( m_verticalLayout );

    m_captionLabel = new ElidedLabel( this );
    m_descriptionLabel = new ElidedLabel( this );
    m_verticalLayout->addWidget( m_captionLabel );
    m_verticalLayout->addWidget( m_descriptionLabel );
    m_verticalLayout->addStretch();

    m_mainLayout->addSpacing( 16 );
    m_mainLayout->setStretchFactor( m_verticalLayout, 2 );

    QPalette pal = palette();
    pal.setColor( QPalette::Foreground, Qt::white );

    m_captionLabel->setPalette( pal );
    m_descriptionLabel->setPalette( pal );

    QFont font = m_captionLabel->font();
    font.setPointSize( TomahawkUtils::defaultFontSize() + 4 );
    font.setBold( true );
    m_captionLabel->setFont( font );
    m_captionLabel->setElideMode( Qt::ElideRight );
    m_captionLabel->setAlignment( Qt::AlignTop | Qt::AlignLeft );

    font.setPointSize( TomahawkUtils::defaultFontSize() + 1 );
    font.setBold( false );
    m_descriptionLabel->setFont( font );
    m_descriptionLabel->setAlignment( Qt::AlignTop | Qt::AlignLeft );

    m_captionLabel->setMargin( 2 );
    m_descriptionLabel->setMargin( 2 );

/*    QGraphicsDropShadowEffect* effect = new QGraphicsDropShadowEffect();
    effect->setBlurRadius( 4 );
    effect->setXOffset( 0 );
    effect->setYOffset( 0 );
    effect->setColor( Qt::white );
    m_captionLabel->setGraphicsEffect( effect );*/
//    m_descriptionLabel->setGraphicsEffect( effect );

    TomahawkUtils::unmarginLayout( layout() );
    layout()->setContentsMargins( 8, 4, 8, 4 );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    setFixedHeight( 80 );

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

    QPainter painter( this );
    painter.setRenderHint( QPainter::Antialiasing );

    QLinearGradient gradient( QPoint( 0, 0 ), QPoint( 0, 1 ) );
    gradient.setCoordinateMode( QGradient::ObjectBoundingMode );
    gradient.setColorAt( 0.0, QColor( "#707070" ) );
    gradient.setColorAt( 1.0, QColor( "#25292c" ) );

    painter.setBrush( gradient );
    painter.fillRect( rect(), gradient );
}
