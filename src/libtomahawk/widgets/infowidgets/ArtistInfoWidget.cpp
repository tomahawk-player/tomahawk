/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2014, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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
#include "ui_HeaderWidget.h"

#include <QDesktopServices>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>

#include "audio/AudioEngine.h"
#include "playlist/GridItemDelegate.h"
#include "playlist/AlbumItemDelegate.h"
#include "playlist/PlayableModel.h"
#include "playlist/TreeModel.h"
#include "playlist/PlaylistModel.h"
#include "playlist/TreeProxyModel.h"
#include "playlist/ContextView.h"
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
    , uiHeader( new Ui::HeaderWidget )
    , m_artist( artist )
{
    m_widget = new QWidget;
    QWidget* headerWidget = new QWidget;
    ui->setupUi( m_widget );
    uiHeader->setupUi( headerWidget );
    headerWidget->setFixedHeight( 160 );

    artist->loadStats();
    connect( artist.data(), SIGNAL( statsLoaded() ), SLOT( onArtistStatsLoaded() ) );

    m_pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultArtistImage, TomahawkUtils::Original, QSize( 48, 48 ) );
    ui->cover->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultArtistImage, TomahawkUtils::Grid, ui->cover->size() ) );

    {
        ui->relatedArtists->setAutoResize( true );
        ui->relatedArtists->setAutoFitItems( true );
        ui->relatedArtists->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        ui->relatedArtists->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        ui->relatedArtists->setItemSize( QSize( 170, 170 + 32 ) );

        m_relatedModel = new PlayableModel( ui->relatedArtists );
        ui->relatedArtists->setPlayableModel( m_relatedModel );
        ui->relatedArtists->proxyModel()->sort( -1 );
        ui->relatedArtists->setEmptyTip( tr( "Sorry, we could not find any related artists!" ) );

        TomahawkStyle::stylePageFrame( ui->relatedArtists );
        TomahawkStyle::stylePageFrame( ui->artistFrame );
        TomahawkStyle::styleScrollBar( ui->relatedArtists->verticalScrollBar() );
    }

    {
        ui->albums->setAutoResize( true );
        ui->albums->setAutoFitItems( false );
        ui->albums->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        ui->albums->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        ui->albums->setWrapping( false );
        ui->albums->setItemSize( QSize( 190, 190 + 56 ) );
        ui->albums->proxyModel()->setHideDupeItems( true );
        ui->albums->setFixedHeight( 190 + 56 + 32 );

        m_albumsModel = new PlayableModel( ui->albums );
        ui->albums->setPlayableModel( m_albumsModel );
        ui->albums->proxyModel()->sort( -1 );
        ui->albums->setEmptyTip( tr( "Sorry, we could not find any albums for this artist!" ) );

        ui->albums->setStyleSheet( QString( "QListView { background-color: white; }" ) );
        TomahawkStyle::stylePageFrame( ui->albumFrame );
        TomahawkStyle::styleScrollBar( ui->albums->verticalScrollBar() );
        TomahawkStyle::styleScrollBar( ui->albums->horizontalScrollBar() );
    }

    {
        ui->topHits->setAutoResize( true );
        ui->topHits->setAutoFitItems( false );
        ui->topHits->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        ui->topHits->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        ui->topHits->setWrapping( false );
        ui->topHits->setItemSize( QSize( 140, 140 + 56 ) );
        ui->topHits->proxyModel()->setHideDupeItems( true );
        ui->topHits->setFixedHeight( 140 + 56 + 32 );

        m_topHitsModel = new PlayableModel( ui->topHits );
        ui->topHits->setPlayableModel( m_topHitsModel );
        ui->topHits->proxyModel()->sort( -1 );
        ui->topHits->setEmptyTip( tr( "Sorry, we could not find any top hits for this artist!" ) );

        ui->topHits->setStyleSheet( QString( "QListView { background-color: white; }" ) );
        TomahawkStyle::stylePageFrame( ui->trackFrame );
    }

    {
        QFont f = ui->biography->font();
        f.setWeight( QFont::Light );
        f.setPointSize( 13 );

        ui->biography->setFont( f );
        ui->biography->setOpenLinks( false );
        ui->biography->setOpenExternalLinks( true );

        QPalette p = ui->biography->palette();
        p.setColor( QPalette::Foreground, Qt::black );
        ui->biography->setPalette( p );

        ui->biography->document()->setDefaultStyleSheet( QString( "a { text-decoration: none; font-weight: bold; color: #000000; }" ) );
        TomahawkStyle::stylePageFrame( ui->biography );
        TomahawkStyle::stylePageFrame( ui->bioFrame );
        TomahawkStyle::styleScrollBar( ui->biography->verticalScrollBar() );

        connect( ui->biography, SIGNAL( anchorClicked( QUrl ) ), SLOT( onBiographyLinkClicked( QUrl ) ) );

        f.setPointSize( 11 );
        ui->topHitsMoreLabel->setFont( f );
        ui->albumsMoreLabel->setFont( f );

        connect( ui->albumsMoreLabel, SIGNAL( clicked() ), SLOT( onAlbumsMoreClicked() ) );
        connect( ui->topHitsMoreLabel, SIGNAL( clicked() ), SLOT( onTopHitsMoreClicked() ) );
    }

    {
        QFont f = uiHeader->artistLabel->font();
        f.setBold( true );
        f.setPointSize( 16 );

        QPalette p = uiHeader->artistLabel->palette();
        p.setColor( QPalette::Foreground, Qt::white );

        uiHeader->artistLabel->setFont( f );
        uiHeader->artistLabel->setPalette( p );

        f.setPointSize( 11 );
        f.setBold( true );
        uiHeader->anchor1Label->setFont( f );
        uiHeader->anchor1Label->setPalette( p );
        uiHeader->anchor2Label->setFont( f );
        uiHeader->anchor2Label->setPalette( p );
        uiHeader->anchor3Label->setFont( f );
        uiHeader->anchor3Label->setPalette( p );

        uiHeader->anchor1Label->setOpacity( 1 );
        uiHeader->anchor2Label->setOpacity( 1 );
        uiHeader->anchor3Label->setOpacity( 1 );
        uiHeader->anchor1Label->setText( tr( "Music" ) );
        uiHeader->anchor2Label->setText( tr( "Biography" ) );
        uiHeader->anchor3Label->setText( tr( "Related Artists" ) );

        QFontMetrics fm( f );
        uiHeader->anchor1Label->setFixedWidth( fm.width( uiHeader->anchor1Label->text() + uiHeader->anchor1Label->contentsMargins().left() * 2 ) );
        uiHeader->anchor2Label->setFixedWidth( fm.width( uiHeader->anchor2Label->text() + uiHeader->anchor1Label->contentsMargins().left() * 2 ) );
        uiHeader->anchor3Label->setFixedWidth( fm.width( uiHeader->anchor3Label->text() + uiHeader->anchor1Label->contentsMargins().left() * 2 ) );

        connect( uiHeader->anchor1Label, SIGNAL( clicked() ), SLOT( onMusicAnchorClicked() ) );
        connect( uiHeader->anchor2Label, SIGNAL( clicked() ), SLOT( onBioAnchorClicked() ) );
        connect( uiHeader->anchor3Label, SIGNAL( clicked() ), SLOT( onRelatedArtistsAnchorClicked() ) );
    }

    m_stackedWidget = new QStackedWidget();

    {
        m_area = new QScrollArea();
        m_area->setWidgetResizable( true );
        m_area->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
        m_area->setWidget( m_widget );

        QPalette pal = palette();
        pal.setBrush( backgroundRole(), Qt::white );
        m_area->setPalette( pal );
        m_area->setAutoFillBackground( true );
        m_area->setFrameShape( QFrame::NoFrame );
        m_area->setAttribute( Qt::WA_MacShowFocusRect, 0 );

        m_stackedWidget->addWidget( m_area );

        connect( m_area->verticalScrollBar(), SIGNAL( valueChanged(int ) ), SLOT( onSliderValueChanged( int ) ) );
    }
    {
        ContextView* topHitsFullView = new ContextView( m_stackedWidget );
        topHitsFullView->setCaption( tr( "Songs" ) );
        topHitsFullView->setShowCloseButton( true );
        topHitsFullView->setPlayableModel( m_topHitsModel );
        m_stackedWidget->addWidget( topHitsFullView );

        connect( topHitsFullView, SIGNAL( closeClicked() ), SLOT( onPageClosed() ) );
    }
    {
        GridView* albumsFullView = new GridView( m_stackedWidget );
        //        albumsFullView->setCaption( tr( "Albums" ) );
        //        albumsFullView->setShowCloseButton( true );
        albumsFullView->setPlayableModel( m_albumsModel );
        albumsFullView->proxyModel()->setHideDupeItems( true );

        CaptionLabel* captionLabel = new CaptionLabel( this );
        captionLabel->setText( tr( "Albums" ) );
        captionLabel->setShowCloseButton( true );

        QWidget* vbox = new QWidget;
        QPalette pal = vbox->palette();
        pal.setBrush( vbox->backgroundRole(), Qt::white );
        vbox->setPalette( pal );
        vbox->setAutoFillBackground( true );

        QVBoxLayout* vboxl = new QVBoxLayout;
        TomahawkUtils::unmarginLayout( vboxl );
        vboxl->setContentsMargins( 32, 32, 32, 32 );
        vboxl->setSpacing( 8 );
        vbox->setLayout( vboxl );

        vboxl->addWidget( captionLabel );
        vboxl->addWidget( albumsFullView );
        vboxl->addStretch();
        vboxl->setStretchFactor( albumsFullView, 1 );

        m_stackedWidget->addWidget( vbox );

        connect( captionLabel, SIGNAL( clicked() ), SLOT( onPageClosed() ) );
    }

    {
        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget( headerWidget );
        layout->addWidget( m_stackedWidget );
        setLayout( layout );
        TomahawkUtils::unmarginLayout( layout );
    }

    MetaPlaylistInterface* mpl = new MetaPlaylistInterface();
    mpl->addChildInterface( ui->relatedArtists->playlistInterface() );
    mpl->addChildInterface( ui->topHits->playlistInterface() );
    mpl->addChildInterface( ui->albums->playlistInterface() );
    m_plInterface = playlistinterface_ptr( mpl );

    onSliderValueChanged( 0 );
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
    uiHeader->artistLabel->setText( artist->name().toUpper() );
    uiHeader->balanceSpacer->changeSize( uiHeader->artistLabel->sizeHint().width(), 1, QSizePolicy::Fixed, QSizePolicy::Fixed );

    connect( m_artist.data(), SIGNAL( biographyLoaded() ), SLOT( onBiographyLoaded() ) );
    connect( m_artist.data(), SIGNAL( similarArtistsLoaded() ), SLOT( onSimilarArtistsLoaded() ) );
    connect( m_artist.data(), SIGNAL( updated() ), SLOT( onArtistImageUpdated() ) );
    connect( m_artist.data(), SIGNAL( albumsAdded( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ),
                                SLOT( onAlbumsFound( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ) );
    connect( m_artist.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                                SLOT( onTracksFound( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode ) ) );

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

//    m_albumsModel->clear();
    m_albumsModel->appendAlbums( albums );
}


void
ArtistInfoWidget::onTracksFound( const QList<Tomahawk::query_ptr>& queries, ModelMode mode )
{
    Q_UNUSED( mode );

    m_topHitsModel->finishLoading();
    m_topHitsModel->appendQueries( queries.mid( 0, 20 ) );
    m_topHitsModel->ensureResolved();
}


void
ArtistInfoWidget::onSimilarArtistsLoaded()
{
    m_relatedModel->appendArtists( m_artist->similarArtists().mid( 0, 20 ) );
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

/*    m_playStatsGauge->setMaximum( m_artist->chartCount() );
    m_playStatsGauge->setValue( m_artist->chartPosition() );*/
}


void
ArtistInfoWidget::onArtistImageUpdated()
{
    if ( m_artist->cover( QSize( 0, 0 ) ).isNull() )
        return;

    m_pixmap = m_artist->cover( QSize( 0, 0 ) );
    emit pixmapChanged( m_pixmap );

    uiHeader->headerWidget->setBackground( m_pixmap, true, false );
    ui->cover->setPixmap( m_artist->cover( QSize( ui->cover->width(), ui->cover->width() ) ) );
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


void
ArtistInfoWidget::onTopHitsMoreClicked()
{
    m_stackedWidget->setCurrentIndex( 1 );
    onSliderValueChanged( 0 );
}


void
ArtistInfoWidget::onAlbumsMoreClicked()
{
    m_stackedWidget->setCurrentIndex( 2 );
    onSliderValueChanged( 0 );
}


void
ArtistInfoWidget::onPageClosed()
{
    m_stackedWidget->setCurrentIndex( 0 );
}


void
ArtistInfoWidget::onMusicAnchorClicked()
{
    m_area->verticalScrollBar()->setValue( 0 );
    onPageClosed();
}


void
ArtistInfoWidget::onBioAnchorClicked()
{
    m_area->verticalScrollBar()->setValue( ui->biographyLabel->mapTo( m_widget, QPoint( 0, 0 ) ).y() - 32 );
    onPageClosed();
}


void
ArtistInfoWidget::onRelatedArtistsAnchorClicked()
{
    m_area->verticalScrollBar()->setValue( ui->relatedArtistsLabel->mapTo( m_widget, QPoint( 0, 0 ) ).y() - 32 );
    onPageClosed();
}


void
ArtistInfoWidget::onSliderValueChanged( int value )
{
    const int midPoint = m_area->viewport()->size().height() / 2;
    const bool bio = ( ( ui->biographyLabel->mapTo( m_widget, QPoint( 0, 0 ) ).y() - 32 ) - value ) < midPoint ;
    const bool ra = ( ( ui->relatedArtistsLabel->mapTo( m_widget, QPoint( 0, 0 ) ).y() - 32 ) - value ) < midPoint;
    const float lowOpacity = 0.8;

    QFont inactive = uiHeader->anchor1Label->font();
    inactive.setBold( false );
    QFont active = uiHeader->anchor1Label->font();
    active.setBold( true );

    if ( ra )
    {
        uiHeader->anchor3Label->setOpacity( 1 );
        uiHeader->anchor1Label->setOpacity( lowOpacity );
        uiHeader->anchor2Label->setOpacity( lowOpacity );

        uiHeader->anchor3Label->setFont( active );
        uiHeader->anchor1Label->setFont( inactive );
        uiHeader->anchor2Label->setFont( inactive );
    }
    else if ( bio )
    {
        uiHeader->anchor2Label->setOpacity( 1 );
        uiHeader->anchor1Label->setOpacity( lowOpacity );
        uiHeader->anchor3Label->setOpacity( lowOpacity );

        uiHeader->anchor2Label->setFont( active );
        uiHeader->anchor1Label->setFont( inactive );
        uiHeader->anchor3Label->setFont( inactive );
    }
    else
    {
        uiHeader->anchor1Label->setOpacity( 1 );
        uiHeader->anchor2Label->setOpacity( lowOpacity );
        uiHeader->anchor3Label->setOpacity( lowOpacity );

        uiHeader->anchor1Label->setFont( active );
        uiHeader->anchor2Label->setFont( inactive );
        uiHeader->anchor3Label->setFont( inactive );
    }
}
