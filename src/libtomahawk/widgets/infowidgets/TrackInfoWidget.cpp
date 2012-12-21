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

    QPalette pal = palette();
    pal.setColor( QPalette::Window, QColor( "#272b2e" ) );

    widget->setPalette( pal );
    widget->setAutoFillBackground( true );

    ui->statsLabel->setStyleSheet( "QLabel { background-image:url(); border: 2px solid #dddddd; background-color: #faf9f9; border-radius: 4px; padding: 12px; }" );
    ui->lyricsView->setStyleSheet( "QTextBrowser#lyricsView { background-color: transparent; }" );

    ui->lyricsView->setFrameShape( QFrame::NoFrame );
    ui->lyricsView->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    ui->lyricsView->setVisible( false ); // FIXME eventually

    ui->similarTracksView->setAutoResize( true );
    ui->similarTracksView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
//    TomahawkUtils::styleScrollBar( ui->similarTracksView->verticalScrollBar() );
    TomahawkUtils::styleScrollBar( ui->lyricsView->verticalScrollBar() );

//    ui->similarTracksView->setStyleSheet( "QListView { background-color: transparent; } QListView::item { background-color: transparent; }" );

    QFont f = ui->statsLabel->font();
    f.setPointSize( TomahawkUtils::defaultFontSize() + 2 );
    f.setBold( true );
    ui->statsLabel->setFont( f );

    QPalette p = ui->lyricsView->palette();
    p.setColor( QPalette::Foreground, Qt::white );
    p.setColor( QPalette::Text, Qt::white );

    ui->lyricsView->setPalette( p );
    ui->label->setPalette( p );
//    ui->similarTracksLabel->setPalette( p );

    m_relatedTracksModel = new PlayableModel( ui->similarTracksView );
    ui->similarTracksView->setPlayableModel( m_relatedTracksModel );
    ui->similarTracksView->proxyModel()->sort( -1 );
    ui->similarTracksView->setEmptyTip( tr( "Sorry, but we could not find similar tracks for this song!" ) );

    m_pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, TomahawkUtils::Original, QSize( 48, 48 ) );
    ui->cover->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultTrackImage, TomahawkUtils::Grid, ui->cover->size() ) );
    ui->cover->setShowText( true );
    
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable( true );
    m_scrollArea->setWidget( widget );
    m_scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );

    m_scrollArea->setStyleSheet( "QScrollArea { background-color: #454e59 }" );
    m_scrollArea->setFrameShape( QFrame::NoFrame );
    m_scrollArea->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget( m_scrollArea );
    setLayout( layout );
    TomahawkUtils::unmarginLayout( layout );

    ui->similarTracksView->setStyleSheet( "QListView { background-color: transparent; }" );
    ui->frame->setStyleSheet( "QFrame#frame { background-color: transparent; }"
                              "QFrame#frame { "
                              "border-image: url(" RESPATH "images/widget-border.png) 3 3 3 3 stretch stretch;"
                              "border-top: 3px transparent; border-bottom: 3px transparent; border-right: 3px transparent; border-left: 3px transparent; }" );

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
    m_query = query;
    m_artist = Artist::get( m_query->artist() );
    m_title = QString( "%1 - %2" ).arg( query->artist() ).arg( query->track() );

    if ( !m_query.isNull() )
    {
        disconnect( m_query.data(), SIGNAL( lyricsLoaded() ), this, SLOT( onLyricsLoaded() ) );
        disconnect( m_query.data(), SIGNAL( similarTracksLoaded() ), this, SLOT( onSimilarTracksLoaded() ) );
        disconnect( m_query.data(), SIGNAL( statsLoaded() ), this, SLOT( onStatsLoaded() ) );
        disconnect( m_query.data(), SIGNAL( updated() ), this, SLOT( onCoverUpdated() ) );
        disconnect( m_artist.data(), SIGNAL( statsLoaded() ), this, SLOT( onStatsLoaded() ) );
        disconnect( m_artist.data(), SIGNAL( similarArtistsLoaded() ), this, SLOT( onSimilarArtistsLoaded() ) );
    }

    connect( m_artist.data(), SIGNAL( similarArtistsLoaded() ), SLOT( onSimilarArtistsLoaded() ) );
    connect( m_artist.data(), SIGNAL( statsLoaded() ), SLOT( onStatsLoaded() ) );
    connect( m_query.data(), SIGNAL( lyricsLoaded() ), SLOT( onLyricsLoaded() ) );
    connect( m_query.data(), SIGNAL( similarTracksLoaded() ), SLOT( onSimilarTracksLoaded() ) );
    connect( m_query.data(), SIGNAL( updated() ), SLOT( onCoverUpdated() ) );
    connect( m_query.data(), SIGNAL( statsLoaded() ), SLOT( onStatsLoaded() ) );

    m_artist->loadStats();
    m_query->loadStats();
//    m_query->lyrics();
    onCoverUpdated();

    ui->cover->setQuery( query );

    m_relatedTracksModel->clear();
    m_relatedTracksModel->startLoading();

    if ( !m_query->similarTracks().isEmpty() )
        onSimilarTracksLoaded();
}


void
TrackInfoWidget::onCoverUpdated()
{
    if ( m_query->cover( QSize( 0, 0 ) ).isNull() )
        return;

    m_pixmap = m_query->cover( ui->cover->size() );
    ui->cover->setPixmap( TomahawkUtils::createRoundedImage( m_pixmap, QSize( 0, 0 ) ) );
}


void
TrackInfoWidget::onStatsLoaded()
{
    QList< Tomahawk::PlaybackLog > history = m_query->playbackHistory( SourceList::instance()->getLocal() );
    const unsigned int trackCounter = m_query->playbackCount( SourceList::instance()->getLocal() );
    const unsigned int artistCounter = m_artist->playbackCount( SourceList::instance()->getLocal() );

    QString stats;

    if ( trackCounter )
        stats = tr( "You've listened to this track %n time(s).", "", trackCounter );
    else
        stats = tr( "You've never listened to this track before." );

    if ( history.count() )
    {
        stats += "\n" + tr( "You first listened to it on %1." ).arg( QDateTime::fromTime_t( history.first().timestamp ).toString( "dd MMM yyyy" ) );
    }

    if ( artistCounter )
        stats += "\n" + tr( "You've listened to %1 %n time(s).", "", artistCounter ).arg( m_artist->name() );
    else
        stats += "\n" + tr( "You've never listened to %1 before." ).arg( m_artist->name() );

    ui->statsLabel->setText( stats );
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
    m_relatedTracksModel->appendQueries( m_query->similarTracks() );
    m_relatedTracksModel->finishLoading();
}


void
TrackInfoWidget::onLyricsLoaded()
{
    ui->lyricsView->setHtml( m_query->lyrics().join( "<br/>" ) );
}


void
TrackInfoWidget::onArtistClicked()
{
    ViewManager::instance()->show( Artist::get( m_query->artist(), false ) );
}


void
TrackInfoWidget::onAlbumClicked()
{
    artist_ptr artist = Artist::get( m_query->artist(), false );
    ViewManager::instance()->show( Album::get( artist, m_query->album(), false ) );
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
