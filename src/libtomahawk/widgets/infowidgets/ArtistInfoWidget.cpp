/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "ArtistInfoWidget.h"
#include "ui_ArtistInfoWidget.h"

#include <QDesktopServices>
#include <QScrollArea>
#include <QScrollBar>

#include "audio/AudioEngine.h"
#include "playlist/GridItemDelegate.h"
#include "playlist/AlbumItemDelegate.h"
#include "playlist/PlayableModel.h"
#include "playlist/TreeModel.h"
#include "playlist/PlaylistModel.h"
#include "playlist/TreeProxyModel.h"
#include "database/DatabaseCommand_AllTracks.h"
#include "database/DatabaseCommand_AllAlbums.h"
#include "Source.h"
#include "GlobalActionManager.h"
#include "Pipeline.h"
#include "SourceList.h"
#include "MetaPlaylistInterface.h"
#include "widgets/StatsGauge.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

using namespace Tomahawk;


ArtistInfoWidget::ArtistInfoWidget( const Tomahawk::artist_ptr& artist, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::ArtistInfoWidget )
    , m_artist( artist )
{
    QWidget* widget = new QWidget;
    ui->setupUi( widget );

    artist->loadStats();
    connect( artist.data(), SIGNAL( statsLoaded() ), SLOT( onArtistStatsLoaded() ) );

    ui->lineAbove->setStyleSheet( QString( "QFrame { border: 1px solid %1; }" ).arg( TomahawkStyle::HEADER_BACKGROUND.name() ) );
    ui->lineBelow->setStyleSheet( QString( "QFrame { border: 1px solid black; }" ) );
    ui->lineAbove2->setStyleSheet( QString( "QFrame { border: 1px solid black; }" ) );
    ui->lineBelow2->setStyleSheet( QString( "QFrame { border: 1px solid %1; }" ).arg( TomahawkStyle::HEADER_BACKGROUND.name() ) );

    QHBoxLayout* l = new QHBoxLayout( ui->statsWidget );
    m_playStatsGauge = new StatsGauge( ui->statsWidget );
    m_playStatsGauge->setText( tr( "YOUR ARTIST RANK" ) );
    m_playStatsGauge->setInvertedAppearance( true );

    l->addSpacerItem( new QSpacerItem( 0, 1, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding ) );
    l->addWidget( m_playStatsGauge );
    l->addSpacerItem( new QSpacerItem( 0, 1, QSizePolicy::Minimum, QSizePolicy::MinimumExpanding ) );
    ui->statsWidget->setLayout( l );
    TomahawkUtils::unmarginLayout( l );

    m_pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultArtistImage, TomahawkUtils::Original, QSize( 48, 48 ) );
    ui->cover->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultArtistImage, TomahawkUtils::Grid, ui->cover->size() ) );
    ui->cover->setShowText( false );

    {
        m_relatedModel = new PlayableModel( ui->relatedArtists );
        ui->relatedArtists->setPlayableModel( m_relatedModel );
        ui->relatedArtists->proxyModel()->sort( -1 );
        ui->relatedArtists->setEmptyTip( tr( "Sorry, we could not find any related artists!" ) );

        ui->relatedArtists->setAutoFitItems( true );
    /*    ui->relatedArtists->setWrapping( false );
        ui->relatedArtists->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        ui->relatedArtists->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );*/
        ui->relatedArtists->delegate()->setItemSize( QSize( 170, 170 ) );

        TomahawkStyle::stylePageFrame( ui->relatedArtists );
        TomahawkStyle::stylePageFrame( ui->artistFrame );
        TomahawkStyle::styleScrollBar( ui->relatedArtists->verticalScrollBar() );
    }

    {
        ui->albums->setAutoResize( true );
        ui->albums->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    /*    ui->albums->setWrapping( false );
        ui->albums->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );*/
        ui->albums->delegate()->setItemSize( QSize( 170, 170 ) );
        ui->albums->proxyModel()->setHideDupeItems( true );

        m_albumsModel = new PlayableModel( ui->albums );
        ui->albums->setPlayableModel( m_albumsModel );
        ui->albums->proxyModel()->sort( -1 );
        ui->albums->setEmptyTip( tr( "Sorry, we could not find any albums for this artist!" ) );

        ui->albums->setStyleSheet( QString( "QListView { background-color: %1; }" ).arg( TomahawkStyle::HEADER_BACKGROUND.name() ) );
        TomahawkStyle::stylePageFrame( ui->albumFrame );
        TomahawkStyle::styleScrollBar( ui->albums->verticalScrollBar() );
    }

    {
        m_topHitsModel = new PlaylistModel( ui->topHits );
        AlbumItemDelegate* del = new AlbumItemDelegate( ui->topHits, ui->topHits->proxyModel() );
        ui->topHits->setPlaylistItemDelegate( del );
        ui->topHits->proxyModel()->setStyle( PlayableProxyModel::Short );
        ui->topHits->setPlayableModel( m_topHitsModel );
        ui->topHits->setSortingEnabled( false );
        ui->topHits->setEmptyTip( tr( "Sorry, we could not find any top hits for this artist!" ) );
        ui->topHits->setAutoResize( true );
        ui->topHits->setAlternatingRowColors( false );

        QPalette p = ui->topHits->palette();
        p.setColor( QPalette::Text, TomahawkStyle::PAGE_TRACKLIST_TRACK_SOLVED );
        p.setColor( QPalette::BrightText, TomahawkStyle::PAGE_TRACKLIST_TRACK_UNRESOLVED );
        p.setColor( QPalette::Foreground, TomahawkStyle::PAGE_TRACKLIST_NUMBER );
        p.setColor( QPalette::Highlight, TomahawkStyle::PAGE_TRACKLIST_HIGHLIGHT );
        p.setColor( QPalette::HighlightedText, TomahawkStyle::PAGE_TRACKLIST_HIGHLIGHT_TEXT );

        ui->topHits->setPalette( p );
        TomahawkStyle::stylePageFrame( ui->topHits );
        TomahawkStyle::stylePageFrame( ui->trackFrame );
    }

    {
        QFont f = ui->biography->font();
        f.setFamily( "Titillium Web" );

        QPalette p = ui->biography->palette();
        p.setColor( QPalette::Text, TomahawkStyle::HEADER_TEXT );

        ui->biography->setFont( f );
        ui->biography->setPalette( p );
        ui->biography->setOpenLinks( false );
        ui->biography->setOpenExternalLinks( true );

        ui->biography->document()->setDefaultStyleSheet( QString( "a { text-decoration: none; font-weight: bold; color: %1; }" ).arg( TomahawkStyle::HEADER_LINK.name() ) );
        TomahawkStyle::stylePageFrame( ui->biography );
        TomahawkStyle::styleScrollBar( ui->biography->verticalScrollBar() );

        connect( ui->biography, SIGNAL( anchorClicked( QUrl ) ), SLOT( onBiographyLinkClicked( QUrl ) ) );
    }

    {
        QFont f = ui->artistLabel->font();
        f.setFamily( "Titillium Web" );

        QPalette p = ui->artistLabel->palette();
        p.setColor( QPalette::Foreground, TomahawkStyle::HEADER_LABEL );

        ui->artistLabel->setFont( f );
        ui->artistLabel->setPalette( p );
    }

    {
        QFont f = ui->label->font();
        f.setFamily( "Pathway Gothic One" );

        QPalette p = ui->label->palette();
        p.setColor( QPalette::Foreground, TomahawkStyle::PAGE_CAPTION );

        ui->label->setFont( f );
        ui->label_2->setFont( f );
        ui->label->setPalette( p );
        ui->label_2->setPalette( p );
    }

    {
        QFont f = ui->albumLabel->font();
        f.setFamily( "Pathway Gothic One" );

        QPalette p = ui->albumLabel->palette();
        p.setColor( QPalette::Foreground, TomahawkStyle::HEADER_TEXT );

        ui->albumLabel->setFont( f );
        ui->albumLabel->setPalette( p );
    }

    {
        QScrollArea* area = new QScrollArea();
        area->setWidgetResizable( true );
        area->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
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

    MetaPlaylistInterface* mpl = new MetaPlaylistInterface();
    mpl->addChildInterface( ui->relatedArtists->playlistInterface() );
    mpl->addChildInterface( ui->topHits->playlistInterface() );
    mpl->addChildInterface( ui->albums->playlistInterface() );
    m_plInterface = playlistinterface_ptr( mpl );

    load( artist );
}


ArtistInfoWidget::~ArtistInfoWidget()
{
    tDebug() << Q_FUNC_INFO;
    delete ui;
}


Tomahawk::playlistinterface_ptr
ArtistInfoWidget::playlistInterface() const
{
    return m_plInterface;
}


bool
ArtistInfoWidget::isBeingPlayed() const
{
    if ( ui->albums && ui->albums->isBeingPlayed() )
        return true;

    if ( ui->relatedArtists && ui->relatedArtists->isBeingPlayed() )
        return true;

    if ( ui->albums && ui->albums->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    if ( ui->relatedArtists && ui->relatedArtists->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    if ( ui->topHits && ui->topHits->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    return false;
}


bool
ArtistInfoWidget::jumpToCurrentTrack()
{
    if ( ui->albums && ui->albums->jumpToCurrentTrack() )
        return true;

    if ( ui->relatedArtists && ui->relatedArtists->jumpToCurrentTrack() )
        return true;

    if ( ui->topHits && ui->topHits->jumpToCurrentTrack() )
        return true;

    if ( ui->albums && ui->albums->jumpToCurrentTrack() )
        return true;

    if ( ui->relatedArtists && ui->relatedArtists->jumpToCurrentTrack() )
        return true;

    return false;
}


void
ArtistInfoWidget::load( const artist_ptr& artist )
{
    if ( m_artist )
    {
        disconnect( m_artist.data(), SIGNAL( updated() ), this, SLOT( onArtistImageUpdated() ) );
        disconnect( m_artist.data(), SIGNAL( similarArtistsLoaded() ), this, SLOT( onSimilarArtistsLoaded() ) );
        disconnect( m_artist.data(), SIGNAL( biographyLoaded() ), this, SLOT( onBiographyLoaded() ) );
        disconnect( m_artist.data(), SIGNAL( albumsAdded( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ),
                    this,              SLOT( onAlbumsFound( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ) );
        disconnect( m_artist.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                    this,              SLOT( onTracksFound( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode ) ) );
    }

    m_artist = artist;
    m_title = artist->name();
    ui->artistLabel->setText( artist->name() );

    connect( m_artist.data(), SIGNAL( biographyLoaded() ), SLOT( onBiographyLoaded() ) );
    connect( m_artist.data(), SIGNAL( similarArtistsLoaded() ), SLOT( onSimilarArtistsLoaded() ) );
    connect( m_artist.data(), SIGNAL( updated() ), SLOT( onArtistImageUpdated() ) );
    connect( m_artist.data(), SIGNAL( albumsAdded( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ),
                                SLOT( onAlbumsFound( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ) );
    connect( m_artist.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                                SLOT( onTracksFound( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode ) ) );

    ui->cover->setArtist( artist );

    m_topHitsModel->startLoading();

    if ( !m_artist->albums( Mixed ).isEmpty() )
        onAlbumsFound( m_artist->albums( Mixed ), Mixed );

    if ( !m_artist->tracks().isEmpty() )
        onTracksFound( m_artist->tracks(), Mixed );

    if ( !m_artist->similarArtists().isEmpty() )
        onSimilarArtistsLoaded();

    if ( !m_artist->biography().isEmpty() )
        onBiographyLoaded();

    onArtistImageUpdated();
}


void
ArtistInfoWidget::onAlbumsFound( const QList<Tomahawk::album_ptr>& albums, ModelMode mode )
{
    Q_UNUSED( mode );

    m_albumsModel->appendAlbums( albums );
}


void
ArtistInfoWidget::onTracksFound( const QList<Tomahawk::query_ptr>& queries, ModelMode mode )
{
    Q_UNUSED( mode );

    m_topHitsModel->finishLoading();
    m_topHitsModel->appendQueries( queries.mid( 0, 20 ) );
}


void
ArtistInfoWidget::onSimilarArtistsLoaded()
{
    m_relatedModel->appendArtists( m_artist->similarArtists() );
}


void
ArtistInfoWidget::onBiographyLoaded()
{
    m_longDescription = m_artist->biography();
    emit longDescriptionChanged( m_longDescription );

    ui->biography->setHtml( m_artist->biography().trimmed().replace( '\n', "<br>" ) );
}


void
ArtistInfoWidget::onArtistStatsLoaded()
{
/*    m_playStatsGauge->setValue( m_artist->playbackCount( SourceList::instance()->getLocal() ) );
    m_playStatsGauge->setMaximum( SourceList::instance()->getLocal()->playbackCount() ); */

    m_playStatsGauge->setMaximum( m_artist->chartCount() );
    m_playStatsGauge->setValue( m_artist->chartPosition() );
}


void
ArtistInfoWidget::onArtistImageUpdated()
{
    if ( m_artist->cover( QSize( 0, 0 ) ).isNull() )
        return;

    m_pixmap = m_artist->cover( QSize( 0, 0 ) );
    emit pixmapChanged( m_pixmap );

    ui->cover->setPixmap( m_artist->cover( QSize( 0, 0 ) ) );
}


void
ArtistInfoWidget::onBiographyLinkClicked( const QUrl& url )
{
    tDebug() << Q_FUNC_INFO << url;

    if ( url.scheme() == "tomahawk" )
    {
        GlobalActionManager::instance()->parseTomahawkLink( url.toString() );
    }
    else
    {
        QDesktopServices::openUrl( url );
    }
}


void
ArtistInfoWidget::changeEvent( QEvent* e )
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
ArtistInfoWidget::pixmap() const
{
    if ( m_pixmap.isNull() )
        return Tomahawk::ViewPage::pixmap();
    else
        return m_pixmap;
}
