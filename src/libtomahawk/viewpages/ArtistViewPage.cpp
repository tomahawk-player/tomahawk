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

#include "ArtistViewPage.h"
#include "ui_ArtistViewPage.h"

#include <QDesktopServices>
#include <QScrollArea>
#include <QScrollBar>
#include <QStackedWidget>
#include <QWebFrame>
#include <QWheelEvent>

#include "audio/AudioEngine.h"
#include "playlist/GridItemDelegate.h"
#include "playlist/PlayableModel.h"
#include "playlist/TreeModel.h"
#include "playlist/PlaylistModel.h"
#include "playlist/TreeProxyModel.h"
#include "playlist/ContextView.h"
#include "playlist/TrackView.h"
#include "database/DatabaseCommand_AllTracks.h"
#include "database/DatabaseCommand_AllAlbums.h"
#include "Source.h"
#include "GlobalActionManager.h"
#include "Pipeline.h"
#include "SourceList.h"
#include "MetaPlaylistInterface.h"
#include "widgets/BasicHeader.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

using namespace Tomahawk;


ArtistInfoWidget::ArtistInfoWidget( const Tomahawk::artist_ptr& artist, QWidget* parent )
    : QWidget( parent )
    , TomahawkUtils::DpiScaler( this )
    , ui( new Ui::ArtistInfoWidget )
    , m_artist( artist )
{
    m_widget = new QWidget;
    m_headerWidget = new BasicHeader;
    ui->setupUi( m_widget );

    {
        ui->relatedArtists->setAutoResize( true );
        ui->relatedArtists->setAutoFitItems( true );
        ui->relatedArtists->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        ui->relatedArtists->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        ui->relatedArtists->setItemWidth( scaledX( 170 ) );

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
        ui->albums->setItemWidth( scaledX( 190 ) );
        ui->albums->proxyModel()->setHideDupeItems( true );
        ui->albums->delegate()->setWordWrapping( true );
        ui->albums->setFixedHeight( ui->albums->itemSize().height() + ui->albums->spacing() * 2 );

        m_albumsModel = new PlayableModel( ui->albums );
        ui->albums->setPlayableModel( m_albumsModel );
        ui->albums->proxyModel()->sort( -1 );
        ui->albums->setEmptyTip( tr( "Sorry, we could not find any albums for this artist!" ) );

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
        ui->topHits->setItemWidth( scaledX( 140 ) );
        ui->topHits->proxyModel()->setHideDupeItems( true );
        ui->topHits->delegate()->setWordWrapping( true );
        ui->topHits->delegate()->setShowBuyButtons( true );
        ui->topHits->setFixedHeight( ui->topHits->itemSize().height() + ui->topHits->spacing() * 2 );

        m_topHitsModel = new PlayableModel( ui->topHits );
        ui->topHits->setPlayableModel( m_topHitsModel );
        ui->topHits->proxyModel()->sort( -1 );
        ui->topHits->setEmptyTip( tr( "Sorry, we could not find any top hits for this artist!" ) );

        TomahawkStyle::stylePageFrame( ui->trackFrame );
    }

    {
        ui->biography->setObjectName( "biography" );
        ui->biography->setContentsMargins( 0, 0, 0, 0 );
        ui->biography->page()->mainFrame()->setScrollBarPolicy( Qt::Horizontal, Qt::ScrollBarAlwaysOff );
        ui->biography->page()->mainFrame()->setScrollBarPolicy( Qt::Vertical, Qt::ScrollBarAsNeeded );
        ui->biography->page()->setLinkDelegationPolicy( QWebPage::DelegateAllLinks );
        ui->biography->installEventFilter( this );

        TomahawkStyle::stylePageWidget( ui->biography );
        TomahawkStyle::stylePageFrame( ui->bioFrame );

        connect( ui->biography, SIGNAL( linkClicked( QUrl ) ), SLOT( onBiographyLinkClicked( QUrl ) ) );

        QFont f = ui->topHitsMoreLabel->font();
        f.setWeight( QFont::Light );
        f.setPointSize( TomahawkUtils::defaultFontSize() + 1 );
        ui->topHitsMoreLabel->setFont( f );
        ui->albumsMoreLabel->setFont( f );

        connect( ui->albumsMoreLabel, SIGNAL( clicked() ), SLOT( onAlbumsMoreClicked() ) );
        connect( ui->topHitsMoreLabel, SIGNAL( clicked() ), SLOT( onTopHitsMoreClicked() ) );

        ui->cover->setFixedSize( scaled( QSize( 384, 384 ) ) );
    }

    {
        m_headerWidget->ui->anchor1Label->setText( tr( "Music" ) );
        m_headerWidget->ui->anchor2Label->setText( tr( "Biography" ) );
        m_headerWidget->ui->anchor3Label->setText( tr( "Related Artists" ) );
        m_headerWidget->ui->anchor1Label->show();
        m_headerWidget->ui->anchor2Label->show();
        m_headerWidget->ui->anchor3Label->show();

        QFontMetrics fm( m_headerWidget->ui->anchor1Label->font() );
        m_headerWidget->ui->anchor1Label->setFixedWidth( fm.width( m_headerWidget->ui->anchor1Label->text() ) + 16 );
        m_headerWidget->ui->anchor2Label->setFixedWidth( fm.width( m_headerWidget->ui->anchor2Label->text() ) + 16 );
        m_headerWidget->ui->anchor3Label->setFixedWidth( fm.width( m_headerWidget->ui->anchor3Label->text() ) + 16 );

        connect( m_headerWidget->ui->anchor1Label, SIGNAL( clicked() ), SLOT( onMusicAnchorClicked() ) );
        connect( m_headerWidget->ui->anchor2Label, SIGNAL( clicked() ), SLOT( onBioAnchorClicked() ) );
        connect( m_headerWidget->ui->anchor3Label, SIGNAL( clicked() ), SLOT( onRelatedArtistsAnchorClicked() ) );
    }

    m_stackedWidget = new QStackedWidget();

    {
        m_area = new QScrollArea();
        m_area->setWidgetResizable( true );
        m_area->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
        m_area->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        m_area->setWidget( m_widget );

        QPalette pal = palette();
        pal.setBrush( backgroundRole(), TomahawkStyle::PAGE_BACKGROUND );
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
        topHitsFullView->trackView()->setPlayableModel( m_topHitsModel );
        m_stackedWidget->addWidget( topHitsFullView );

        connect( topHitsFullView, SIGNAL( closeClicked() ), SLOT( onPageClosed() ) );
    }
    {
        GridView* albumsFullView = new GridView( m_stackedWidget );
        albumsFullView->delegate()->setWordWrapping( true );
        //        albumsFullView->setCaption( tr( "Albums" ) );
        //        albumsFullView->setShowCloseButton( true );
        albumsFullView->setPlayableModel( m_albumsModel );
        albumsFullView->proxyModel()->setHideDupeItems( true );

        CaptionLabel* captionLabel = new CaptionLabel( this );
        captionLabel->setText( tr( "Albums" ) );
        captionLabel->setShowCloseButton( true );

        QWidget* vbox = new QWidget;
        QPalette pal = vbox->palette();
        pal.setBrush( vbox->backgroundRole(), TomahawkStyle::PAGE_BACKGROUND );
        vbox->setPalette( pal );
        vbox->setAutoFillBackground( true );

        QVBoxLayout* vboxl = new QVBoxLayout;
        TomahawkUtils::unmarginLayout( vboxl );
        vboxl->setContentsMargins( 32, 32, 32, 32 );
        vboxl->setSpacing( scaledY( 8 ) );
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
        layout->addWidget( m_headerWidget );
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

    TomahawkUtils::fixMargins( this );

    m_pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultArtistImage, TomahawkUtils::Original, scaled( QSize( 48, 48 ) ) );
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
    m_headerWidget->setCaption( artist->name() );

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

    onArtistImageUpdated();
    ui->biography->setFixedHeight( ui->cover->width() );

    QString html =
        QString( "<html><head><style type=text/css>"
                 "body { margin: 0; padding: 0; color: #333333; background-color: #ffffff; font-family: Roboto; font-size: %1pt; font-weight: normal; }"
                    "a { color: #000000; text-decoration: none; font-weight: bold; }"
                    "a:hover { color: #000000; text-decoration: underline; font-weight: bold; }"
                 "</style></head>"
                 "<body>%2</body></html>" )
               .arg( TomahawkUtils::defaultFontSize() )
               .arg( m_artist->biography().trimmed().replace( '\n', "<br><br>" ) );

    ui->biography->setHtml( html );
}


void
ArtistInfoWidget::onArtistImageUpdated()
{
    const QSize coverSize = QSize( ui->cover->width(), ui->cover->width() );
    if ( !m_artist || m_artist->cover( QSize( 0, 0 ) ).isNull() )
    {
        ui->cover->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultArtistImage, TomahawkUtils::Original, coverSize ) );
    }
    else
    {
        ui->cover->setPixmap( m_artist->cover( coverSize ) );
    }

    m_pixmap = m_artist->cover( QSize( 0, 0 ) );
    emit pixmapChanged( m_pixmap );

    m_headerWidget->setBackground( m_pixmap, true, false );
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
    QFont inactive = m_headerWidget->ui->anchor1Label->font();
    inactive.setBold( false );
    QFont active = m_headerWidget->ui->anchor1Label->font();
    active.setBold( true );

    if ( ra )
    {
        m_headerWidget->ui->anchor3Label->setOpacity( 1 );
        m_headerWidget->ui->anchor1Label->setOpacity( lowOpacity );
        m_headerWidget->ui->anchor2Label->setOpacity( lowOpacity );

        m_headerWidget->ui->anchor3Label->setFont( active );
        m_headerWidget->ui->anchor1Label->setFont( inactive );
        m_headerWidget->ui->anchor2Label->setFont( inactive );
    }
    else if ( bio )
    {
        m_headerWidget->ui->anchor2Label->setOpacity( 1 );
        m_headerWidget->ui->anchor1Label->setOpacity( lowOpacity );
        m_headerWidget->ui->anchor3Label->setOpacity( lowOpacity );

        m_headerWidget->ui->anchor2Label->setFont( active );
        m_headerWidget->ui->anchor1Label->setFont( inactive );
        m_headerWidget->ui->anchor3Label->setFont( inactive );
    }
    else
    {
        m_headerWidget->ui->anchor1Label->setOpacity( 1 );
        m_headerWidget->ui->anchor2Label->setOpacity( lowOpacity );
        m_headerWidget->ui->anchor3Label->setOpacity( lowOpacity );

        m_headerWidget->ui->anchor1Label->setFont( active );
        m_headerWidget->ui->anchor2Label->setFont( inactive );
        m_headerWidget->ui->anchor3Label->setFont( inactive );
    }
}


bool
ArtistInfoWidget::eventFilter( QObject* obj, QEvent* event )
{
    if ( event->type() == QEvent::Wheel )
    {
        QWheelEvent* we = static_cast<QWheelEvent*>( event );
        QWheelEvent* wheelEvent = new QWheelEvent(
            we->pos(),
            we->globalPos(),
            we->delta(),
            we->buttons(),
            we->modifiers(),
            we->orientation() );

        qApp->postEvent( m_area->viewport(), wheelEvent );
        event->accept();
        return true;
    }
    else
        return QObject::eventFilter( obj, event );
}


void
ArtistInfoWidget::resizeEvent( QResizeEvent* event )
{
    QWidget::resizeEvent( event );
}
