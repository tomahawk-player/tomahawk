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
#include "TomahawkSettings.h"

#include <QLayout>
#include <QScrollArea>
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

bool
WhatsNew_0_8::addPageItem() const
{
    return !TomahawkSettings::instance()->value( "whatsnew/deleted-for-0.8", false ).toBool();
}


void
WhatsNew_0_8::onItemDeleted()
{
    TomahawkSettings::instance()->setValue( "whatsnew/deleted-for-0.8", true );
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
        QScrollArea* area = new QScrollArea();
        area->setWidgetResizable( true );
        area->setWidget( widget );

        QPalette pal = palette();
        pal.setBrush( backgroundRole(), TomahawkStyle::HEADER_BACKGROUND );
        area->setPalette( pal );
        area->setAutoFillBackground( true );
        area->setFrameShape( QFrame::NoFrame );
        area->setAttribute( Qt::WA_MacShowFocusRect, 0 );

        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget( area );
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
        connect( ui->designBoxHeader, SIGNAL( clicked() ), SLOT( designBoxClicked() ) );
        connect( ui->designBoxImage, SIGNAL( clicked() ), SLOT( designBoxClicked() ) );
    }

    {
        connect( ui->androidBoxHeader, SIGNAL( clicked() ), SLOT( androidBoxClicked() ) );
        connect( ui->androidBoxImage, SIGNAL( clicked() ), SLOT( androidBoxClicked() ) );
    }

    {
        QFont font = ui->label_2->font();

        int fontSize = TomahawkUtils::defaultFontSize() + 2;
        font.setPointSize( fontSize );
        font.setFamily( "Titillium Web" );

        ui->label_2->setFont( font );
        ui->label_3->setFont( font );
        ui->label_5->setFont( font );
        ui->label_7->setFont( font );
        ui->label_9->setFont( font );
        ui->label_11->setFont( font );
        ui->label_13->setFont( font );
        ui->label_17->setFont( font );
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
    activateBox( ui->inboxBox, 0 );
}


void
WhatsNewWidget_0_8::urlLookupBoxClicked()
{
    activateBox( ui->urlLookupBox, 1 );
}


void
WhatsNewWidget_0_8::trendingBoxClicked()
{
    activateBox( ui->trendingBox, 2 );
}


void
WhatsNewWidget_0_8::beatsBoxClicked()
{
    activateBox( ui->beatsBox, 3 );
}


void
WhatsNewWidget_0_8::gmusicBoxClicked()
{
    activateBox( ui->gmusicBox, 4 );
}


void
WhatsNewWidget_0_8::networkingBoxClicked()
{
    activateBox( ui->networkingBox, 5 );
}


void
WhatsNewWidget_0_8::designBoxClicked()
{
    activateBox( ui->designBox, 6 );
}


void
WhatsNewWidget_0_8::androidBoxClicked()
{
    activateBox( ui->androidBox, 8 );
}


void
WhatsNewWidget_0_8::activateBox( QWidget* widget, int activeIndex )
{
    deactivateAllBoxes();

    widget->layout()->setContentsMargins( 8, 8, 8, 16 );
    widget->setStyleSheet( activeWidgetThumbStylesheet );

    ui->stackedWidget->setCurrentIndex( activeIndex );
}


void
WhatsNewWidget_0_8::deactivateBox( QWidget* widget )
{
    widget->layout()->setContentsMargins( 8, 8, 8, 8 );
    widget->setStyleSheet( inactiveWidgetThumbStylesheet );
}


void
WhatsNewWidget_0_8::deactivateAllBoxes()
{
    deactivateBox( ui->inboxBox );
    deactivateBox( ui->urlLookupBox );
    deactivateBox( ui->trendingBox );
    deactivateBox( ui->beatsBox );
    deactivateBox( ui->gmusicBox );
    deactivateBox( ui->networkingBox );
    deactivateBox( ui->designBox );
    deactivateBox( ui->androidBox );
}


Q_EXPORT_PLUGIN2( ViewPagePlugin, WhatsNew_0_8 )
