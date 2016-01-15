/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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
#include "utils/ImageRegistry.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QLabel>
#include <QPixmap>
#include <QPaintEvent>
#include <QPainter>
#include <QPushButton>
#include <QBoxLayout>

using namespace Tomahawk;


BasicHeader::BasicHeader( QWidget* parent )
    : BackgroundWidget( parent )
    , DpiScaler( this )
    , ui( new Ui::HeaderWidget )
{
    ui->setupUi( this );
    setAutoFillBackground( false );
    BackgroundWidget::setBackgroundColor( TomahawkStyle::HEADER_BACKGROUND );

    ui->refreshButton->setPixmap( ImageRegistry::instance()->pixmap( RESPATH "images/refresh.svg", QSize( ui->captionLabel->height() - 8, ui->captionLabel->height() - 8 ) ) );
    connect( ui->refreshButton, SIGNAL( clicked() ), SIGNAL( refresh() ) );
    setRefreshVisible( false );

    {
        QFont f = ui->captionLabel->font();
        f.setPointSize( TomahawkUtils::defaultFontSize() + 6 );
        f.setLetterSpacing( QFont::PercentageSpacing, 110 );

        QPalette p = ui->captionLabel->palette();
        p.setColor( QPalette::Foreground, TomahawkStyle::HEADER_LABEL );

        ui->captionLabel->setFont( f );
        ui->captionLabel->setPalette( p );
        ui->captionLabel->setWordWrap( true );

        ui->iconLabel->hide();

        ui->anchor1Label->hide();
        ui->anchor2Label->hide();
        ui->anchor3Label->hide();
    }

    {
        QFont f = ui->anchor1Label->font();
        f.setPointSize( TomahawkUtils::defaultFontSize() + 1 );
        f.setBold( true );

        QPalette p = ui->captionLabel->palette();
        p.setColor( QPalette::Foreground, Qt::white );

        ui->anchor1Label->setFont( f );
        ui->anchor1Label->setPalette( p );
        ui->anchor2Label->setFont( f );
        ui->anchor2Label->setPalette( p );
        ui->anchor3Label->setFont( f );
        ui->anchor3Label->setPalette( p );
    }

    setFixedHeight( scaledY( 130 ) );
    TomahawkUtils::fixMargins( this );
}


BasicHeader::~BasicHeader()
{
}


void
BasicHeader::setCaption( const QString& s )
{
    ui->captionLabel->setText( s.toUpper() );
    resizeEvent( 0 );
}


void
BasicHeader::setDescription( const QString& /* s */ )
{
    //FIXME
//    m_descriptionLabel->setText( s );
}


void
BasicHeader::setRefreshVisible( bool visible )
{
    ui->refreshButton->setVisible( visible );
}


void
BasicHeader::setPixmap( const QPixmap& pixmap, bool tinted )
{
    QFontMetrics fm( ui->captionLabel->font() );
    ui->iconLabel->setFixedHeight( fm.ascent() );

    QPixmap p = pixmap;
    if ( tinted )
        p = TomahawkUtils::tinted( p, Qt::white );
    ui->iconLabel->setPixmap( p.scaledToHeight( ui->iconLabel->height(), Qt::SmoothTransformation ) );

    if ( !p.isNull() )
        ui->iconLabel->show();
}


QPushButton*
BasicHeader::addButton( const QString& text )
{
    QPushButton* button = new QPushButton( this );

    button->setStyleSheet( "QPushButton:hover { font-size: 12px; color: #2b2b2b; background: #f8f8f8; border-style: solid; border-radius: 0px; border-width: 2px; border-color: #2b2b2b; }"
                           "QPushButton { font-size: 12px; color: #ffffff; background-color: #000000; border-style: solid; border-radius: 0px; border-width: 0px; }" );
    button->setMinimumHeight( 30 );
    button->setMinimumWidth( 132 );

    button->setText( text );

    ui->horizontalLayout->addSpacing( 8 );
    ui->horizontalLayout->addWidget( button );

    return button;
}


void
BasicHeader::resizeEvent( QResizeEvent* event )
{
    BackgroundWidget::resizeEvent( event );

    QFontMetrics fm( ui->captionLabel->font() );
    ui->captionLabel->setFixedWidth( qMin( fm.width( ui->captionLabel->text() ) + 8, int( width() * 0.33 ) ) );
    ui->balanceSpacer->changeSize( ui->captionLabel->width(), 1, QSizePolicy::Expanding, QSizePolicy::Fixed );
}
