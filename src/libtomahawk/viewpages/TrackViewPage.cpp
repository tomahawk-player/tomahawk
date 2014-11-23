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

#include "TrackViewPage.h"
#include "ui_TrackViewPage.h"

#include <QScrollArea>
#include <QScrollBar>

#include "ViewManager.h"
#include "SourceList.h"
#include "playlist/TrackView.h"
#include "playlist/PlayableModel.h"
#include "audio/AudioEngine.h"
#include "widgets/BasicHeader.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

using namespace Tomahawk;


TrackInfoWidget::TrackInfoWidget( const Tomahawk::query_ptr& query, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::TrackInfoWidget )
{
    QWidget* widget = new QWidget;
    m_headerWidget = new BasicHeader;
    ui->setupUi( widget );

    m_pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, TomahawkUtils::Original, QSize( 48, 48 ) );

    m_relatedTracksModel = new PlayableModel( ui->trackView );
    ui->trackView->trackView()->setPlayableModel( m_relatedTracksModel );
    ui->trackView->setCaption( tr( "Similar Tracks" ) );
    ui->trackView->setEmptyTip( tr( "Sorry, but we could not find similar tracks for this song!" ) );

    ui->topHits->setStyleSheet( QString( "QListView { background-color: #f9f9f9; }" ) );
    TomahawkStyle::stylePageFrame( ui->trackFrame );
    ui->topHits->setVisible( false );
    ui->topHitsLabel->setVisible( false );

    {
        QScrollArea* area = new QScrollArea();
        area->setWidgetResizable( true );
        area->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        area->setWidget( widget );

        QPalette pal = palette();
        pal.setBrush( backgroundRole(), TomahawkStyle::PAGE_BACKGROUND );
        area->setPalette( pal );
        area->setAutoFillBackground( true );
        area->setFrameShape( QFrame::NoFrame );
        area->setAttribute( Qt::WA_MacShowFocusRect, 0 );

        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget( m_headerWidget );
        layout->addWidget( area );
        setLayout( layout );
        TomahawkUtils::unmarginLayout( layout );
    }

    load( query );
}


TrackInfoWidget::~TrackInfoWidget()
{
    tDebug() << Q_FUNC_INFO;
    delete ui;
}


Tomahawk::playlistinterface_ptr
TrackInfoWidget::playlistInterface() const
{
    return ui->trackView->playlistInterface();
}


bool
TrackInfoWidget::isBeingPlayed() const
{
    if ( ui->trackView->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    if ( ui->trackView->playlistInterface()->hasChildInterface( AudioEngine::instance()->currentTrackPlaylist() ) )
        return true;

    return false;
}


bool
TrackInfoWidget::jumpToCurrentTrack()
{
    return ui->trackView && ui->trackView->jumpToCurrentTrack();
}


void
TrackInfoWidget::load( const query_ptr& query )
{
    if ( m_query )
    {
        disconnect( m_query->track().data(), SIGNAL( lyricsLoaded() ), this, SLOT( onLyricsLoaded() ) );
        disconnect( m_query->track().data(), SIGNAL( similarTracksLoaded() ), this, SLOT( onSimilarTracksLoaded() ) );
        disconnect( m_query->track().data(), SIGNAL( updated() ), this, SLOT( onCoverUpdated() ) );
    }

    m_query = query;
    m_title = QString( "%1 - %2" ).arg( query->track()->track() ).arg( query->track()->artist() );
    m_headerWidget->setCaption( m_title );

    connect( m_query->track().data(), SIGNAL( lyricsLoaded() ), SLOT( onLyricsLoaded() ) );
    connect( m_query->track().data(), SIGNAL( similarTracksLoaded() ), SLOT( onSimilarTracksLoaded() ) );
    connect( m_query->track().data(), SIGNAL( updated() ), SLOT( onCoverUpdated() ) );

//    m_query->lyrics();
    onCoverUpdated();

    m_relatedTracksModel->clear();
    m_relatedTracksModel->startLoading();

    if ( !m_query->track()->similarTracks().isEmpty() )
        onSimilarTracksLoaded();
}


void
TrackInfoWidget::onCoverUpdated()
{
    if ( m_query->track()->cover( QSize( 0, 0 ) ).isNull() )
        return;

    m_pixmap = m_query->track()->cover( QSize( 0, 0 ) );
    emit pixmapChanged( m_pixmap );

    m_headerWidget->setBackground( m_pixmap, true, false );
}


void
TrackInfoWidget::onSimilarTracksLoaded()
{
    m_relatedTracksModel->appendQueries( m_query->track()->similarTracks() );
    m_relatedTracksModel->ensureResolved();
}


void
TrackInfoWidget::onLyricsLoaded()
{
//    ui->lyricsView->setHtml( m_query->track()->lyrics().join( "<br/>" ) );
}


void
TrackInfoWidget::changeEvent( QEvent* e )
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
TrackInfoWidget::pixmap() const
{
    if ( m_pixmap.isNull() )
        return Tomahawk::ViewPage::pixmap();
    else
        return m_pixmap;
}
