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

#include <QtGui/QLabel>
#include <QtGui/QPixmap>
#include <QtGui/QPaintEvent>
#include <QtGui/QPainter>
#include <QtGui/QBoxLayout>

#include "utils/TomahawkUtilsGui.h"
#include "ElidedLabel.h"

using namespace Tomahawk;

QPixmap* BasicHeader::s_tiledHeader = 0;


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
    m_descriptionLabel->setMargin( 1 );

    TomahawkUtils::unmarginLayout( layout() );
    layout()->setContentsMargins( 8, 4, 8, 4 );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed );
    setFixedHeight( 80 );

    pal = palette();
    pal.setColor( QPalette::Window, QColor( "#454e59" ) );

    setPalette( pal );
    setAutoFillBackground( true );
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
