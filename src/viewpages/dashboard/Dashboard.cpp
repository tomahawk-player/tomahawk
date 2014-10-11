/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
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

#include "Dashboard.h"
#include "ui_DashboardWidget.h"

#include "libtomahawk-widgets/PlaylistDelegate.h"

#include "ViewManager.h"
#include "SourceList.h"
#include "TomahawkSettings.h"
#include "widgets/RecentPlaylistsModel.h"
#include "widgets/RecentlyPlayedPlaylistsModel.h"
#include "MetaPlaylistInterface.h"
#include "audio/AudioEngine.h"
#include "playlist/TrackView.h"
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

#define HISTORY_PLAYLIST_ITEMS 10
#define HISTORY_TRACK_ITEMS 15

using namespace Tomahawk;
using namespace Tomahawk::Widgets;


Dashboard::Dashboard( QWidget* /* parent */ )
{
}


Dashboard::~Dashboard()
{
}


DashboardWidget::DashboardWidget( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::DashboardWidget )
{
    QWidget* widget = new QWidget;
    BasicHeader* headerWidget = new BasicHeader;
    headerWidget->setCaption( tr( "Feed" ) );
    ui->setupUi( widget );

    {
        m_tracksModel = new RecentlyPlayedModel( ui->trackView->trackView(), HISTORY_TRACK_ITEMS );
        ui->trackView->setPlayableModel( m_tracksModel );
        ui->trackView->setCaption( tr( "Recently Played Tracks" ) );
        ui->trackView->trackView()->setUniformRowHeights( false );
        ui->trackView->trackView()->setIndentation( 0 );

//        ui->trackView->trackView()->setAutoResize( true );
        m_tracksModel->setSource( source_ptr() );
    }

    {
/*        QScrollArea* area = new QScrollArea();
        area->setWidgetResizable( true );
        area->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        area->setWidget( widget );

        QPalette pal = palette();
        pal.setBrush( backgroundRole(), Qt::white );
        area->setPalette( pal );
        area->setAutoFillBackground( true );
        area->setFrameShape( QFrame::NoFrame );
        area->setAttribute( Qt::WA_MacShowFocusRect, 0 );*/

        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget( headerWidget );
        layout->addWidget( widget );
        setLayout( layout );
        TomahawkUtils::unmarginLayout( layout );
    }

    MetaPlaylistInterface* mpl = new MetaPlaylistInterface();
    mpl->addChildInterface( ui->trackView->trackView()->playlistInterface() );
    m_playlistInterface = playlistinterface_ptr( mpl );

    connect( ui->trackView, SIGNAL( pixmapChanged( QPixmap ) ), headerWidget, SLOT( setBackground( QPixmap ) ) );
}


DashboardWidget::~DashboardWidget()
{
    delete ui;
}


playlistinterface_ptr
DashboardWidget::playlistInterface() const
{
    return m_playlistInterface;
}


bool
DashboardWidget::jumpToCurrentTrack()
{
    if ( ui->trackView->trackView()->jumpToCurrentTrack() )
        return true;

    return false;
}


bool
DashboardWidget::isBeingPlayed() const
{
    return AudioEngine::instance()->currentTrackPlaylist() == ui->trackView->trackView()->playlistInterface();
}


void
DashboardWidget::changeEvent( QEvent* e )
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


Q_EXPORT_PLUGIN2( ViewPagePlugin, Dashboard )
