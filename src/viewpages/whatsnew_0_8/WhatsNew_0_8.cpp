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

#include "libtomahawk-widgets/PlaylistDelegate.h"

#include "ViewManager.h"
#include "SourceList.h"
#include "TomahawkSettings.h"
#include "widgets/RecentPlaylistsModel.h"
#include "widgets/RecentlyPlayedPlaylistsModel.h"
#include "MetaPlaylistInterface.h"
#include "audio/AudioEngine.h"
#include "playlist/AlbumModel.h"
#include "playlist/RecentlyPlayedModel.h"
#include "playlist/dynamic/GeneratorInterface.h"
#include "widgets/OverlayWidget.h"
#include "widgets/BasicHeader.h"
#include "utils/ImageRegistry.h"
#include "utils/AnimatedSpinner.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "utils/DpiScaler.h"

#include <QPainter>
#include <QScrollArea>
#include <QLabel>

#define HISTORY_PLAYLIST_ITEMS 10
#define HISTORY_TRACK_ITEMS 15

using namespace Tomahawk;
using namespace Tomahawk::Widgets;


WhatsNew_0_8::WhatsNew_0_8( QWidget* parent )
{
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
    }

    {
        QPixmap urlLookupPixmap = ImageRegistry::instance()->pixmap( RESPATH "images/drop-all-songs.svg", QSize( 64, 64) );
        ui->urlLookupBoxImage->setPixmap( urlLookupPixmap );
    }

    {
        QPixmap trendingPixmap = ImageRegistry::instance()->pixmap( RESPATH "images/trending.svg", QSize( 64, 64) );
        ui->trendingBoxImage->setPixmap( trendingPixmap );
    }

    {
        // TODO: Add Beats Music Pixmap
    }

    {
        // TODO: Add GMusic Pixmap
    }

    {
        // TODO: Add IPv6 Pixmap
    }

    {
        // TODO: Add Design Pixmap
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


Q_EXPORT_PLUGIN2( ViewPagePlugin, WhatsNew_0_8 )
