/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2014, Uwe L. Korn <uwelk@xhochy.com>
 *   Copyright 2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "utils/Logger.h"
#include "utils/ImageRegistry.h"
#include "utils/TomahawkStyle.h"
#include "TomahawkSettings.h"

#include <QLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>

using namespace Tomahawk;
using namespace Tomahawk::Widgets;


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
    m_widget = new QWidget;
    ui->setupUi( m_widget );

    {
        m_area = new QScrollArea();
        m_area->setWidgetResizable( true );
        m_area->setWidget( m_widget );

        QPalette pal = palette();
        pal.setBrush( backgroundRole(), Qt::white );
        m_area->setPalette( pal );
        m_area->setAutoFillBackground( true );
        m_area->setFrameShape( QFrame::NoFrame );
        m_area->setAttribute( Qt::WA_MacShowFocusRect, 0 );

        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget( m_area );
        setLayout( layout );
        TomahawkUtils::unmarginLayout( layout );
    }

    int width = ui->widget->minimumSize().width() - ( ui->widget->layout()->contentsMargins().left() + ui->widget->layout()->contentsMargins().right() );
    {
        ui->inboxImage->setPixmap( ui->inboxImage->pixmap()->scaledToWidth( width, Qt::SmoothTransformation ) );
        ui->inboxImage->setFixedHeight( ui->inboxImage->pixmap()->height() );
        ui->inboxButton->setFixedSize( QSize( 80, 80 ) );
        ui->inboxButton->setCursor( Qt::PointingHandCursor );
        QPixmap inboxPixmap = ImageRegistry::instance()->pixmap( ":/whatsnew_0_8/data/images/inboxbutton.png", ui->inboxButton->size() );
        ui->inboxButton->setPixmap( inboxPixmap );
        connect( ui->inboxButton, SIGNAL( clicked() ), SLOT( inboxBoxClicked() ) );
    }

    {
        ui->linkImage->setPixmap( ui->linkImage->pixmap()->scaledToWidth( width, Qt::SmoothTransformation ) );
        ui->linkImage->setFixedHeight( ui->linkImage->pixmap()->height() );
        ui->linkButton->setFixedSize( QSize( 80, 80 ) );
        ui->linkButton->setCursor( Qt::PointingHandCursor );
        QPixmap pixmap = ImageRegistry::instance()->pixmap( ":/whatsnew_0_8/data/images/connectivitybutton.png", ui->linkButton->size() );
        ui->linkButton->setPixmap( pixmap );
        connect( ui->linkButton, SIGNAL( clicked() ), SLOT( urlLookupBoxClicked() ) );
    }

    {
        ui->beatsImage->setPixmap( ui->beatsImage->pixmap()->scaledToWidth( width, Qt::SmoothTransformation ) );
        ui->beatsImage->setFixedHeight( ui->beatsImage->pixmap()->height() );
        ui->beatsButton->setFixedSize( QSize( 80, 80 ) );
        ui->beatsButton->setCursor( Qt::PointingHandCursor );
        QPixmap beatsPixmap = ImageRegistry::instance()->pixmap( ":/whatsnew_0_8/data/images/beatsbutton.png", ui->beatsButton->size() );
        ui->beatsButton->setPixmap( beatsPixmap );
        connect( ui->beatsButton, SIGNAL( clicked() ), SLOT( beatsBoxClicked() ) );
    }

    {
        ui->googleImage->setPixmap( ui->googleImage->pixmap()->scaledToWidth( width, Qt::SmoothTransformation ) );
        ui->googleImage->setFixedHeight( ui->googleImage->pixmap()->height() );
        ui->googleButton->setFixedSize( QSize( 80, 80 ) );
        ui->googleButton->setCursor( Qt::PointingHandCursor );
        QPixmap pixmap = ImageRegistry::instance()->pixmap( ":/whatsnew_0_8/data/images/googlebutton.png", ui->googleButton->size() );
        ui->googleButton->setPixmap( pixmap );
        connect( ui->googleButton, SIGNAL( clicked() ), SLOT( gmusicBoxClicked() ) );
    }

    {
        ui->androidImage->setPixmap( ui->androidImage->pixmap()->scaledToWidth( width, Qt::SmoothTransformation ) );
        ui->androidImage->setFixedHeight( ui->androidImage->pixmap()->height() );
        ui->androidButton->setFixedSize( QSize( 80, 80 ) );
        ui->androidButton->setCursor( Qt::PointingHandCursor );
        QPixmap pixmap = ImageRegistry::instance()->pixmap( ":/whatsnew_0_8/data/images/androidbutton.png", ui->androidButton->size() );
        ui->androidButton->setPixmap( pixmap );
        connect( ui->androidButton, SIGNAL( clicked() ), SLOT( androidBoxClicked() ) );
    }

    {
        ui->networkImage->setPixmap( ui->networkImage->pixmap()->scaledToWidth( width, Qt::SmoothTransformation ) );
        ui->networkImage->setFixedHeight( ui->networkImage->pixmap()->height() );
        ui->networkButton->setFixedSize( QSize( 80, 80 ) );
        ui->networkButton->setCursor( Qt::PointingHandCursor );
        QPixmap networkingPixmap = ImageRegistry::instance()->pixmap( ":/whatsnew_0_8/data/images/networkbutton.png", ui->networkButton->size() );
        ui->networkButton->setPixmap( networkingPixmap );
        connect( ui->networkButton, SIGNAL( clicked() ), SLOT( networkingBoxClicked() ) );
    }

    {
        QFont font = ui->label->font();
        font.setWeight( QFont::Light );
        font.setPointSize( TomahawkUtils::defaultFontSize() + 38 );
        ui->label->setFont( font );
        ui->label->setStyleSheet( "QLabel { color: rgba( 0, 0, 0, 60% ) }" );

        font.setWeight( QFont::Normal );
        font.setPointSize( TomahawkUtils::defaultFontSize() + 1 );
        ui->label_2->setFont( font );
        ui->label_2->setStyleSheet( "QLabel { color: rgba( 0, 0, 0, 30% ) }" );

        font.setPointSize( TomahawkUtils::defaultFontSize() + 2 );
        ui->label_3->setFont( font );
        ui->label_3->setStyleSheet( "QLabel { color: rgba( 0, 0, 0, 65% ) }" );
        ui->label_5->setFont( font );
        ui->label_5->setStyleSheet( "QLabel { color: rgba( 0, 0, 0, 65% ) }" );
        ui->label_6->setFont( font );
        ui->label_6->setStyleSheet( "QLabel { color: rgba( 0, 0, 0, 65% ) }" );
        ui->label_7->setFont( font );
        ui->label_7->setStyleSheet( "QLabel { color: rgba( 0, 0, 0, 65% ) }" );
        ui->label_8->setFont( font );
        ui->label_8->setStyleSheet( "QLabel { color: rgba( 0, 0, 0, 65% ) }" );
        ui->label_9->setFont( font );
        ui->label_9->setStyleSheet( "QLabel { color: rgba( 0, 0, 0, 65% ) }" );
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
    activateAnchor( ui->inboxCaption );
}


void
WhatsNewWidget_0_8::urlLookupBoxClicked()
{
    activateAnchor( ui->linkCaption );
}


void
WhatsNewWidget_0_8::beatsBoxClicked()
{
    activateAnchor( ui->beatsCaption );
}


void
WhatsNewWidget_0_8::gmusicBoxClicked()
{
    activateAnchor( ui->googleCaption );
}


void
WhatsNewWidget_0_8::networkingBoxClicked()
{
    activateAnchor( ui->networkCaption );
}


void
WhatsNewWidget_0_8::androidBoxClicked()
{
    activateAnchor( ui->androidCaption );
}


void
WhatsNewWidget_0_8::activateAnchor( QWidget* widget )
{
    m_area->verticalScrollBar()->setValue( widget->mapTo( m_widget, QPoint( 0, 0 ) ).y() - 32 );
}


Q_EXPORT_PLUGIN2( ViewPagePlugin, WhatsNew_0_8 )
