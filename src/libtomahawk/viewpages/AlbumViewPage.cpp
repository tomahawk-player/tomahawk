/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2012, Jeff Mitchell <jeff@tomahawk-player.org>
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "AlbumViewPage.h"
#include "ui_AlbumViewPage.h"

#include "audio/AudioEngine.h"
#include "ViewManager.h"
#include "database/Database.h"
#include "playlist/TreeModel.h"
#include "playlist/PlayableModel.h"
#include "Source.h"
#include "MetaPlaylistInterface.h"
#include "playlist/TrackView.h"
#include "playlist/TrackDetailView.h"
#include "widgets/BasicHeader.h"

#include "database/DatabaseCommand_AllTracks.h"
#include "database/DatabaseCommand_AllAlbums.h"

#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QScrollArea>
#include <QScrollBar>

using namespace Tomahawk;


AlbumInfoWidget::AlbumInfoWidget( const Tomahawk::album_ptr& album, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::AlbumInfoWidget )
{
    QWidget* widget = new QWidget;
    m_headerWidget = new BasicHeader;
    ui->setupUi( widget );

    m_pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultAlbumCover, TomahawkUtils::Original, QSize( 48, 48 ) );

    m_tracksModel = new TreeModel();
    m_tracksModel->setMode( Mixed );

    // We need to set the model on the view before loading the playlist, so spinners & co are connected
    ui->albumView->trackDetailView()->setBuyButtonVisible( true );
    ui->albumView->trackView()->setPlayableModel( m_tracksModel );
    ui->albumView->setCaption( tr( "Album Details" ) );

/*    ui->topHits->setStyleSheet( QString( "QListView { background-color: %1; }" ).arg( TomahawkStyle::PAGE_BACKGROUND.name() ) );
    TomahawkStyle::stylePageFrame( ui->trackFrame );
    ui->topHits->setVisible( false );
    ui->topHitsLabel->setVisible( false );*/

    {
/*        QScrollArea* area = new QScrollArea();
        area->setWidgetResizable( true );
        area->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        area->setWidget( widget );

        QPalette pal = palette();
        pal.setBrush( backgroundRole(), TomahawkStyle::PAGE_BACKGROUND );
        area->setPalette( pal );
        area->setAutoFillBackground( true );
        area->setFrameShape( QFrame::NoFrame );
        area->setAttribute( Qt::WA_MacShowFocusRect, 0 );*/

        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget( m_headerWidget );
        layout->addWidget( widget );
        setLayout( layout );
        TomahawkUtils::unmarginLayout( layout );
    }

    MetaPlaylistInterface* mpl = new MetaPlaylistInterface();
//    mpl->addChildInterface( ui->topHits->playlistInterface() );
    mpl->addChildInterface( ui->albumView->playlistInterface() );
    m_playlistInterface = playlistinterface_ptr( mpl );

    load( album );
}


AlbumInfoWidget::~AlbumInfoWidget()
{
    tDebug() << Q_FUNC_INFO;
    delete ui;
}


Tomahawk::playlistinterface_ptr
AlbumInfoWidget::playlistInterface() const
{
    return m_playlistInterface;
}


bool
AlbumInfoWidget::isBeingPlayed() const
{
    if ( ui->albumView && ui->albumView->isBeingPlayed() )
        return true;

/*    if ( ui->topHits && ui->topHits->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;*/

    return false;
}


bool
AlbumInfoWidget::jumpToCurrentTrack()
{
    return ui->albumView && ui->albumView->jumpToCurrentTrack();
}


void
AlbumInfoWidget::load( const album_ptr& album )
{
    if ( !m_album.isNull() )
    {
        disconnect( m_album.data(), SIGNAL( updated() ), this, SLOT( onAlbumImageUpdated() ) );
    }

    m_album = album;
    m_title = album->name();

    connect( m_album.data(), SIGNAL( updated() ), SLOT( onAlbumImageUpdated() ) );

    m_headerWidget->setCaption( album->artist()->name() );

    m_tracksModel->addTracks( album, QModelIndex() );

    onAlbumImageUpdated();
}


void
AlbumInfoWidget::onAlbumImageUpdated()
{
    if ( m_album->cover( QSize( 0, 0 ) ).isNull() )
        return;

    m_pixmap = m_album->cover( QSize( 0, 0 ) );
    emit pixmapChanged( m_pixmap );

    m_headerWidget->setBackground( m_pixmap, true, false );
}


void
AlbumInfoWidget::changeEvent( QEvent* e )
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


QPixmap
AlbumInfoWidget::pixmap() const
{
    if ( m_pixmap.isNull() )
        return Tomahawk::ViewPage::pixmap();
    else
        return m_pixmap;
}