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

#include "ViewManager.h"
#include "SourceList.h"
#include "playlist/AlbumModel.h"

#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"

using namespace Tomahawk;


TrackInfoWidget::TrackInfoWidget( const Tomahawk::query_ptr& query, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::TrackInfoWidget )
    , m_infoId( uuid() )
{
    ui->setupUi( this );

    layout()->setSpacing( 0 );
    ui->headerWidget->setStyleSheet( "QWidget#headerWidget { background-image: url(" RESPATH "images/playlist-header-tiled.png); }" );
    ui->tracksWidget->setStyleSheet( "QWidget#tracksWidget{background-color: #323435;}" );
    ui->statsLabel->setStyleSheet( "QLabel { background-image:url(); border: 2px solid #dddddd; background-color: #faf9f9; border-radius: 4px; padding: 12px; }" );

    QFont f = font();
    f.setBold( true );
    f.setPixelSize( 18 );
    ui->trackLabel->setFont( f );
    ui->similarTracksLabel->setFont( f );

    f.setPixelSize( 14 );
    ui->artistLabel->setFont( f );
    ui->albumLabel->setFont( f );
    ui->byLabel->setFont( f );
    ui->fromLabel->setFont( f );
    
    f.setPixelSize( 12 );
    ui->statsLabel->setFont( f );

    ui->similarTracksView->setFrameShape( QFrame::NoFrame );
    ui->similarTracksView->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    ui->similarTracksView->setStyleSheet( "QListView { background-color: transparent; } QListView::item { background-color: transparent; }" );

    QPalette p = ui->trackLabel->palette();
    p.setColor( QPalette::Foreground, Qt::white );
    ui->trackLabel->setPalette( p );
    ui->artistLabel->setPalette( p );
    ui->albumLabel->setPalette( p );
    ui->byLabel->setPalette( p );
    ui->fromLabel->setPalette( p );

    m_albumsModel = new AlbumModel( ui->similarTracksView );
    ui->similarTracksView->setAlbumModel( m_albumsModel );
    ui->similarTracksView->proxyModel()->sort( -1 );

    m_pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultAlbumCover, TomahawkUtils::ScaledCover, QSize( 48, 48 ) );

    load( query );
}


TrackInfoWidget::~TrackInfoWidget()
{
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
    //tDebug() << Q_FUNC_INFO << "audioengine playlistInterface = " << AudioEngine::instance()->currentTrackPlaylist()->id();
    //tDebug() << Q_FUNC_INFO << "albumsView playlistInterface = " << ui->albumsView->playlistInterface()->id();
    //tDebug() << Q_FUNC_INFO << "tracksView playlistInterface = " << ui->tracksView->playlistInterface()->id();
    if ( ui->similarTracksView->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

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
        disconnect( m_query.data(), SIGNAL( statsLoaded() ), this, SLOT( onStatsLoaded() ) );
        disconnect( m_query.data(), SIGNAL( updated() ), this, SLOT( onCoverUpdated() ) );
        disconnect( m_artist.data(), SIGNAL( statsLoaded() ), this, SLOT( onStatsLoaded() ) );
        disconnect( m_artist.data(), SIGNAL( similarArtistsLoaded() ), this, SLOT( onSimilarArtistsLoaded() ) );
    }

    connect( m_artist.data(), SIGNAL( similarArtistsLoaded() ), SLOT( onSimilarArtistsLoaded() ) );
    connect( m_artist.data(), SIGNAL( statsLoaded() ), SLOT( onStatsLoaded() ) );
    connect( m_query.data(), SIGNAL( similarTracksLoaded() ), SLOT( onSimilarTracksLoaded() ) );
    connect( m_query.data(), SIGNAL( updated() ), SLOT( onCoverUpdated() ) );
    connect( m_query.data(), SIGNAL( statsLoaded() ), SLOT( onStatsLoaded() ) );

    m_artist->loadStats();
    m_query->loadStats();
    onCoverUpdated();

    ui->trackLabel->setText( query->track() );
    ui->artistLabel->setText( query->artist() );
    ui->albumLabel->setText( query->album() );
    ui->fromLabel->setVisible( !query->album().isEmpty() );

    m_query->similarTracks();
    m_albumsModel->addArtists( m_artist->similarArtists() );
    m_albumsModel->clear();
}


void
TrackInfoWidget::onCoverUpdated()
{
    if ( m_query->cover( QSize( 0, 0 ) ).isNull() )
        return;

    m_pixmap = m_query->cover( ui->cover->size() );
    ui->cover->setPixmap( m_pixmap );
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
    Artist* artist = qobject_cast<Artist*>( sender() );

//    m_albumsModel->addArtists( artist->similarArtists() );
}


void
TrackInfoWidget::onSimilarTracksLoaded()
{
    m_albumsModel->addQueries( m_query->similarTracks() );
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
