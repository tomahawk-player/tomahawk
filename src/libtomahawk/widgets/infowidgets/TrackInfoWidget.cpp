/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "TrackInfoWidget.h"
#include "ui_TrackInfoWidget.h"

#include <QScrollArea>
#include <QScrollBar>

#include "ViewManager.h"
#include "SourceList.h"
#include "playlist/PlayableModel.h"
#include "audio/AudioEngine.h"
#include "widgets/StatsGauge.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

using namespace Tomahawk;


TrackInfoWidget::TrackInfoWidget( const Tomahawk::query_ptr& query, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::TrackInfoWidget )
    , m_scrollArea( 0 )
{
    QWidget* widget = new QWidget;
    ui->setupUi( widget );

    ui->statsLabel->setStyleSheet( "QLabel { background-image:url(); border: 2px solid #dddddd; background-color: #faf9f9; border-radius: 4px; padding: 12px; }" );
    ui->statsLabel->setVisible( false );

/*    ui->lyricsView->setVisible( false ); // FIXME eventually
    TomahawkStyle::stylePageFrame( ui->lyricsView );
    TomahawkStyle::styleScrollBar( ui->lyricsView->verticalScrollBar() );*/

    ui->lineAbove->setStyleSheet( QString( "QFrame { border: 1px solid %1; }" ).arg( TomahawkStyle::HEADER_BACKGROUND.name() ) );
    ui->lineBelow->setStyleSheet( QString( "QFrame { border: 1px solid black; }" ) );

    m_pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, TomahawkUtils::Original, QSize( 48, 48 ) );
    ui->cover->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, TomahawkUtils::Grid, ui->cover->size() ) );
    ui->cover->setShowText( false );

    QHBoxLayout* l = new QHBoxLayout( ui->statsWidget );
    m_playStatsGauge = new StatsGauge( ui->statsWidget );
    m_playStatsGauge->setText( tr( "# PLAYS / ARTIST" ) );
    m_playStatsTotalGauge = new StatsGauge( ui->statsWidget );
    m_playStatsTotalGauge->setText( tr( "YOUR SONG RANK" ) );
    m_playStatsTotalGauge->setInvertedAppearance( true );

    l->addSpacerItem( new QSpacerItem( 0, 1, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding ) );
    l->addWidget( m_playStatsGauge );
    l->addSpacerItem( new QSpacerItem( 0, 1, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding ) );
    l->addWidget( m_playStatsTotalGauge );
    l->addSpacerItem( new QSpacerItem( 0, 1, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding ) );
    ui->statsWidget->setLayout( l );
    TomahawkUtils::unmarginLayout( l );

    {
        m_relatedTracksModel = new PlayableModel( ui->similarTracksView );
        ui->similarTracksView->setPlayableModel( m_relatedTracksModel );
        ui->similarTracksView->proxyModel()->sort( -1 );
        ui->similarTracksView->setEmptyTip( tr( "Sorry, but we could not find similar tracks for this song!" ) );
        ui->similarTracksView->setAutoResize( true );
        ui->similarTracksView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        //    TomahawkUtils::styleScrollBar( ui->similarTracksView->verticalScrollBar() );
        //    ui->similarTracksView->setStyleSheet( "QListView { background-color: transparent; } QListView::item { background-color: transparent; }" );

        TomahawkStyle::stylePageFrame( ui->similarTracksView );
        TomahawkStyle::stylePageFrame( ui->frame );
    }

    {
        QFont f = ui->trackLabel->font();
        f.setFamily( "Titillium Web" );

        QPalette p = ui->trackLabel->palette();
        p.setColor( QPalette::Foreground, TomahawkStyle::HEADER_LABEL );

        ui->trackLabel->setFont( f );
        ui->trackLabel->setPalette( p );
    }

    {
        ui->artistLabel->setContentsMargins( 6, 2, 6, 2 );
        ui->artistLabel->setElideMode( Qt::ElideMiddle );
        ui->artistLabel->setType( QueryLabel::Artist );
        connect( ui->artistLabel, SIGNAL( clickedArtist() ), SLOT( onArtistClicked() ) );

        QFont f = ui->artistLabel->font();
        f.setFamily( "Titillium Web" );

        QPalette p = ui->artistLabel->palette();
        p.setColor( QPalette::Foreground, TomahawkStyle::HEADER_TEXT );

        ui->artistLabel->setFont( f );
        ui->artistLabel->setPalette( p );
    }

    {
        QFont f = ui->label->font();
        f.setFamily( "Pathway Gothic One" );

        QPalette p = ui->label->palette();
        p.setColor( QPalette::Foreground, TomahawkStyle::PAGE_CAPTION );

        ui->label->setFont( f );
        ui->label->setPalette( p );
    }

    {
        QFont f = ui->statsLabel->font();
        f.setPointSize( TomahawkUtils::defaultFontSize() + 2 );
        ui->statsLabel->setFont( f );
    }

    /*{
        QPalette p = ui->lyricsView->palette();
        p.setColor( QPalette::Foreground, TomahawkStyle::PAGE_FOREGROUND );
        p.setColor( QPalette::Text, TomahawkStyle::PAGE_FOREGROUND );
        ui->lyricsView->setPalette( p );
    }*/

    {
        m_scrollArea = new QScrollArea();
        m_scrollArea->setWidgetResizable( true );
        m_scrollArea->setWidget( widget );
        m_scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );

        QPalette pal = palette();
        pal.setBrush( backgroundRole(), TomahawkStyle::HEADER_BACKGROUND );
        m_scrollArea->setPalette( pal );
        m_scrollArea->setAutoFillBackground( true );
        m_scrollArea->setFrameShape( QFrame::NoFrame );
        m_scrollArea->setAttribute( Qt::WA_MacShowFocusRect, 0 );

        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget( m_scrollArea );
        setLayout( layout );
        TomahawkUtils::unmarginLayout( layout );
    }

    {
        QPalette pal = palette();
        pal.setBrush( backgroundRole(), TomahawkStyle::PAGE_BACKGROUND );
        ui->widget->setPalette( pal );
        ui->widget->setAutoFillBackground( true );
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
    return ui->similarTracksView->playlistInterface();
}


bool
TrackInfoWidget::isBeingPlayed() const
{
    if ( ui->similarTracksView->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    if ( ui->similarTracksView->playlistInterface()->hasChildInterface( AudioEngine::instance()->currentTrackPlaylist() ) )
        return true;

    return false;
}


bool
TrackInfoWidget::jumpToCurrentTrack()
{
    if ( ui->similarTracksView->jumpToCurrentTrack() && !ui->similarTracksView->currentTrackRect().isEmpty() )
    {
        // We embed the view in a scrollarea, so we have to manually ensure we make it visible
        const QRect itemRect = ui->similarTracksView->currentTrackRect();
        m_scrollArea->ensureVisible( itemRect.right(), itemRect.bottom(), 50, 50 );

        return true;
    }

    return false;
}


void
TrackInfoWidget::load( const query_ptr& query )
{
    if ( !m_query.isNull() )
    {
        disconnect( m_query->track().data(), SIGNAL( lyricsLoaded() ), this, SLOT( onLyricsLoaded() ) );
        disconnect( m_query->track().data(), SIGNAL( similarTracksLoaded() ), this, SLOT( onSimilarTracksLoaded() ) );
        disconnect( m_query->track().data(), SIGNAL( statsLoaded() ), this, SLOT( onStatsLoaded() ) );
        disconnect( m_query->track().data(), SIGNAL( updated() ), this, SLOT( onCoverUpdated() ) );
        disconnect( m_artist.data(), SIGNAL( statsLoaded() ), this, SLOT( onStatsLoaded() ) );
        disconnect( m_artist.data(), SIGNAL( similarArtistsLoaded() ), this, SLOT( onSimilarArtistsLoaded() ) );
    }

    m_query = query;
    m_artist = Artist::get( m_query->track()->artist() );
    m_title = QString( "%1 - %2" ).arg( query->track()->artist() ).arg( query->track()->track() );
    ui->trackLabel->setText( m_query->track()->track() );
    ui->artistLabel->setArtist( m_query->track()->artistPtr() );

    connect( m_artist.data(), SIGNAL( similarArtistsLoaded() ), SLOT( onSimilarArtistsLoaded() ) );
    connect( m_artist.data(), SIGNAL( statsLoaded() ), SLOT( onStatsLoaded() ) );
    connect( m_query->track().data(), SIGNAL( lyricsLoaded() ), SLOT( onLyricsLoaded() ) );
    connect( m_query->track().data(), SIGNAL( similarTracksLoaded() ), SLOT( onSimilarTracksLoaded() ) );
    connect( m_query->track().data(), SIGNAL( updated() ), SLOT( onCoverUpdated() ) );
    connect( m_query->track().data(), SIGNAL( statsLoaded() ), SLOT( onStatsLoaded() ) );

    m_artist->loadStats();
    m_query->track()->loadStats();
//    m_query->lyrics();
    onCoverUpdated();

    ui->cover->setQuery( query );

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

    m_pixmap = m_query->track()->cover( ui->cover->size() );
    ui->cover->setPixmap( TomahawkUtils::createRoundedImage( m_pixmap, QSize( 0, 0 ) ) );
}


void
TrackInfoWidget::onStatsLoaded()
{
    QString stats;
    QList< Tomahawk::PlaybackLog > history = m_query->track()->playbackHistory( SourceList::instance()->getLocal() );
    const unsigned int trackCounter = m_query->track()->playbackCount( SourceList::instance()->getLocal() );
    const unsigned int artistCounter = m_artist->playbackCount( SourceList::instance()->getLocal() );

    if ( trackCounter )
        stats = tr( "You've listened to this track %n time(s).", "", trackCounter );
    else
        stats = tr( "You've never listened to this track before." );

    if ( history.count() )
    {
        stats += "\n" + tr( "You first listened to it on %1." ).arg( QDateTime::fromTime_t( history.first().timestamp ).toString( "dd MMM yyyy" ) );
    }

    if ( artistCounter )
    {
        stats += "\n" + tr( "You've listened to %1 %n time(s).", "", artistCounter ).arg( m_artist->name() );

        m_playStatsGauge->setMaximum( artistCounter );
        m_playStatsGauge->setValue( trackCounter );
    }
    else
        stats += "\n" + tr( "You've never listened to %1 before." ).arg( m_artist->name() );

    ui->statsLabel->setText( stats );
    m_playStatsTotalGauge->setMaximum( m_query->track()->chartCount() );
    m_playStatsTotalGauge->setValue( m_query->track()->chartPosition() );
}


void
TrackInfoWidget::onSimilarArtistsLoaded()
{
/*    Artist* artist = qobject_cast<Artist*>( sender() );

    m_relatedArtistsModel->addArtists( artist->similarArtists() );*/
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
TrackInfoWidget::onArtistClicked()
{
    ViewManager::instance()->show( m_query->track()->artistPtr() );
}


void
TrackInfoWidget::onAlbumClicked()
{
    ViewManager::instance()->show( m_query->track()->albumPtr() );
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
