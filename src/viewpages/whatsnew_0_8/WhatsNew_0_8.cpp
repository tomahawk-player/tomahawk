/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Uwe L. Korn <uwelk@xhochy.com>
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

#include "WhatsNew_0_8.h"
#include "ui_WhatsNewWidget_0_8.h"

#include "utils/ImageRegistry.h"
#include "utils/TomahawkStyle.h"

#include <QLayout>
#include <QStackedWidget>

using namespace Tomahawk;
using namespace Tomahawk::Widgets;

const char* activeWidgetThumbStylesheet = "QWidget {"
        "border-width: 2px;"
        "border-style: solid;"
        "border-radius: 4px;"
        "border-color: white;"
        "color: white;"
        "border-bottom: none;"
        "border-bottom-right-radius: 0px;"
        "border-bottom-left-radius: 0px;"
        "background-color:#292f34;"
        "}";

const char* inactiveWidgetThumbStylesheet = " QWidget {"
        "border-width: 2px;"
        "border-style: solid;"
        "border-radius: 4px;"
        "border-color: grey;"
        "color: grey;"
        "}";

WhatsNew_0_8::WhatsNew_0_8( QWidget* parent )
{
    Q_UNUSED( parent );
}


WhatsNew_0_8::~WhatsNew_0_8()
{

}


WhatsNewWidget_0_8::WhatsNewWidget_0_8( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::WhatsNewWidget_0_8 )
{
    QWidget* widget = new QWidget;
    ui->setupUi( widget );

    ui->lineAbove->setStyleSheet( QString( "QFrame { border: 1px solid black; }" ) );
    ui->lineBelow->setStyleSheet( QString( "QFrame { border: 1px solid %1; }" ).arg( TomahawkStyle::HEADER_BACKGROUND.name() ) );

    {
        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget( widget );
        setLayout( layout );
        TomahawkUtils::unmarginLayout( layout );
    }

    {
        QPalette pal = palette();
        pal.setBrush( backgroundRole(), TomahawkStyle::PAGE_BACKGROUND );
        ui->widget->setPalette( pal );
        ui->widget->setAutoFillBackground( true );
    }

    {
        QPixmap inboxPixmap = ImageRegistry::instance()->pixmap( RESPATH "images/inbox.svg", QSize( 64, 64 ) );
        ui->inboxBoxImage->setPixmap( inboxPixmap );

        connect( ui->inboxBoxHeader, SIGNAL( clicked() ), SLOT( inboxBoxClicked() ) );
        connect( ui->inboxBoxImage, SIGNAL( clicked() ), SLOT( inboxBoxClicked() ) );
    }

    {
        QPixmap urlLookupPixmap = ImageRegistry::instance()->pixmap( RESPATH "images/drop-all-songs.svg", QSize( 64, 64 ) );
        ui->urlLookupBoxImage->setPixmap( urlLookupPixmap );

        connect( ui->urlLookupBoxHeader, SIGNAL( clicked() ), SLOT( urlLookupBoxClicked() ) );
        connect( ui->urlLookupBoxImage, SIGNAL( clicked() ), SLOT( urlLookupBoxClicked() ) );
    }

    {
        QPixmap trendingPixmap = ImageRegistry::instance()->pixmap( RESPATH "images/trending.svg", QSize( 64, 64 ) );
        ui->trendingBoxImage->setPixmap( trendingPixmap );

        connect( ui->trendingBoxHeader, SIGNAL( clicked() ), SLOT( trendingBoxClicked() ) );
        connect( ui->trendingBoxImage, SIGNAL( clicked() ), SLOT( trendingBoxClicked() ) );
    }

    {
        QPixmap beatsPixmap = ImageRegistry::instance()->pixmap( RESPATH "images/beatsmusic.svg", QSize( 64, 64 ) );
        ui->beatsBoxImage->setPixmap( beatsPixmap );

        connect( ui->beatsBoxHeader, SIGNAL( clicked() ), SLOT( beatsBoxClicked() ) );
        connect( ui->beatsBoxImage, SIGNAL( clicked() ), SLOT( beatsBoxClicked() ) );
    }

    {
        // TODO: Add GMusic Pixmap

        connect( ui->gmusicBoxHeader, SIGNAL( clicked() ), SLOT( gmusicBoxClicked() ) );
        connect( ui->gmusicBoxImage, SIGNAL( clicked() ), SLOT( gmusicBoxClicked() ) );
    }

    {
        QPixmap networkingPixmap = ImageRegistry::instance()->pixmap( RESPATH "images/ipv6-logo.svg", QSize( 64, 64 ) );
        ui->networkingBoxImage->setPixmap( networkingPixmap );

        connect( ui->networkingBoxHeader, SIGNAL( clicked() ), SLOT( networkingBoxClicked() ) );
        connect( ui->networkingBoxImage, SIGNAL( clicked() ), SLOT( networkingBoxClicked() ) );
    }

    {
        QPixmap designPixmap = ImageRegistry::instance()->pixmap( RESPATH "images/new-additions.svg", QSize( 64, 64 ) );
        ui->designBoxImage->setPixmap( designPixmap );

        connect( ui->designBoxHeader, SIGNAL( clicked() ), SLOT( designBoxClicked() ) );
        connect( ui->designBoxImage, SIGNAL( clicked() ), SLOT( designBoxClicked() ) );
    }
}


WhatsNewWidget_0_8::~WhatsNewWidget_0_8()
{
    delete ui;
}


playlistinterface_ptr
WhatsNewWidget_0_8::playlistInterface() const
{
    return playlistinterface_ptr();
}


bool
WhatsNewWidget_0_8::jumpToCurrentTrack()
{
    return false;
}


bool
WhatsNewWidget_0_8::isBeingPlayed() const
{
    return false;
}


void
WhatsNewWidget_0_8::changeEvent( QEvent* e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


void
WhatsNewWidget_0_8::inboxBoxClicked()
{
    deactivateAllBoxes();
    activateBox( ui->inboxBox );
    ui->stackedWidget->setCurrentIndex( 0 );
}


void
WhatsNewWidget_0_8::urlLookupBoxClicked()
{
    deactivateAllBoxes();
    activateBox( ui->urlLookupBox );
    ui->stackedWidget->setCurrentIndex( 1 );
}


void
WhatsNewWidget_0_8::trendingBoxClicked()
{
    deactivateAllBoxes();
    activateBox( ui->trendingBox );
    ui->stackedWidget->setCurrentIndex( 2 );
}


void
WhatsNewWidget_0_8::beatsBoxClicked()
{
    deactivateAllBoxes();
    activateBox( ui->beatsBox );
    ui->stackedWidget->setCurrentIndex( 3 );
}


void
WhatsNewWidget_0_8::gmusicBoxClicked()
{
    deactivateAllBoxes();
    activateBox( ui->gmusicBox );
    ui->stackedWidget->setCurrentIndex( 4 );
}


void
WhatsNewWidget_0_8::networkingBoxClicked()
{
    deactivateAllBoxes();
    activateBox( ui->networkingBox );
    ui->stackedWidget->setCurrentIndex( 5 );
}


void
WhatsNewWidget_0_8::designBoxClicked()
{
    deactivateAllBoxes();
    activateBox( ui->designBox );
    ui->stackedWidget->setCurrentIndex( 6 );
}


void
WhatsNewWidget_0_8::activateBox( QWidget* widget )
{
    widget->layout()->setContentsMargins( 8, 8, 8, 16 );
    widget->setStyleSheet( activeWidgetThumbStylesheet );
}


void
WhatsNewWidget_0_8::deactivateAllBoxes()
{
    ui->inboxBox->layout()->setContentsMargins( 8, 8, 8, 8 );
    ui->inboxBox->setStyleSheet( inactiveWidgetThumbStylesheet );

    ui->urlLookupBox->layout()->setContentsMargins( 8, 8, 8, 8 );
    ui->urlLookupBox->setStyleSheet( inactiveWidgetThumbStylesheet );

    ui->trendingBox->layout()->setContentsMargins( 8, 8, 8, 8 );
    ui->trendingBox->setStyleSheet( inactiveWidgetThumbStylesheet );

    ui->beatsBox->layout()->setContentsMargins( 8, 8, 8, 8 );
    ui->beatsBox->setStyleSheet( inactiveWidgetThumbStylesheet );

    ui->gmusicBox->layout()->setContentsMargins( 8, 8, 8, 8 );
    ui->gmusicBox->setStyleSheet( inactiveWidgetThumbStylesheet );

    ui->networkingBox->layout()->setContentsMargins( 8, 8, 8, 8 );
    ui->networkingBox->setStyleSheet( inactiveWidgetThumbStylesheet );

    ui->designBox->layout()->setContentsMargins( 8, 8, 8, 8 );
    ui->designBox->setStyleSheet( inactiveWidgetThumbStylesheet );
}


Q_EXPORT_PLUGIN2( ViewPagePlugin, WhatsNew_0_8 )
