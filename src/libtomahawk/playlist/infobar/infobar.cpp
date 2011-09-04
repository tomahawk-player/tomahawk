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

#include "infobar.h"
#include "ui_infobar.h"

#include <QLabel>
#include <QPixmap>

#include "context/ContextWidget.h"
#include "utils/tomahawkutils.h"
#include "utils/logger.h"

#define ANIMATION_TIME 400
#define IMAGE_HEIGHT 64

using namespace Tomahawk;


InfoBar::InfoBar( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::InfoBar )
{
    ui->setupUi( this );
    TomahawkUtils::unmarginLayout( layout() );
    layout()->setContentsMargins( 8, 4, 8, 4 );

    QFont boldFont = ui->captionLabel->font();
    boldFont.setPixelSize( 18 );
    boldFont.setBold( true );
    ui->captionLabel->setFont( boldFont );
    ui->captionLabel->setElideMode( Qt::ElideRight );

    boldFont.setPixelSize( 12 );
    ui->descriptionLabel->setFont( boldFont );

    QFont regFont = ui->longDescriptionLabel->font();
    regFont.setPixelSize( 11 );
    ui->longDescriptionLabel->setFont( regFont );

    QPalette whitePal = ui->captionLabel->palette();
    whitePal.setColor( QPalette::Foreground, Qt::white );

    ui->captionLabel->setPalette( whitePal );
    ui->descriptionLabel->setPalette( whitePal );
    ui->longDescriptionLabel->setPalette( whitePal );

    ui->captionLabel->setMargin( 6 );
    ui->descriptionLabel->setMargin( 6 );
    ui->longDescriptionLabel->setMargin( 4 );

    ui->captionLabel->setText( QString() );
    ui->descriptionLabel->setText( QString() );
    ui->longDescriptionLabel->setText( QString() );
    ui->imageLabel->setText( QString() );

    setAutoFillBackground( true );
}


InfoBar::~InfoBar()
{
    delete ui;
}


void
InfoBar::setCaption( const QString& s )
{
    ui->captionLabel->setText( s );
}


void
InfoBar::setDescription( const QString& s )
{
    ui->descriptionLabel->setText( s );
}


void
InfoBar::setLongDescription( const QString& s )
{
    ui->longDescriptionLabel->setText( s );

    if ( s.isEmpty() )
    {
        ui->horizontalLayout->setStretchFactor( ui->verticalLayout, 1 );
        ui->horizontalLayout->setStretchFactor( ui->verticalLayout_2, 0 );
    } else
    {
        ui->horizontalLayout->setStretchFactor( ui->verticalLayout, 0 );
        ui->horizontalLayout->setStretchFactor( ui->verticalLayout_2, 99 );
    }
}


void
InfoBar::setPixmap( const QPixmap& p )
{
    ui->imageLabel->setPixmap( p.scaledToHeight( IMAGE_HEIGHT, Qt::SmoothTransformation ) );
}


void
InfoBar::changeEvent( QEvent* e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
//            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


void
InfoBar::resizeEvent( QResizeEvent* e )
{
    QWidget::resizeEvent( e );

    QLinearGradient gradient = QLinearGradient( contentsRect().topLeft(), contentsRect().bottomRight() );
    gradient.setColorAt( 0.0, QColor( 100, 100, 100 ) );
    gradient.setColorAt( 1.0, QColor( 63, 63, 63 ) );

    QPalette p = palette();
    p.setBrush( QPalette::Window, QBrush( gradient ) );
    setPalette( p );
}
