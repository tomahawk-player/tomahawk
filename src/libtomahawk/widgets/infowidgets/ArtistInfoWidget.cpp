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
#include "ArtistInfoWidget_p.h"
#include "ui_ArtistInfoWidget.h"

#include <QScrollArea>
#include <QScrollBar>

#include "audio/AudioEngine.h"
#include "playlist/GridItemDelegate.h"
#include "playlist/PlayableModel.h"
#include "playlist/TreeModel.h"
#include "playlist/PlaylistModel.h"
#include "playlist/TreeProxyModel.h"
#include "Source.h"

#include "database/DatabaseCommand_AllTracks.h"
#include "database/DatabaseCommand_AllAlbums.h"

#include "Pipeline.h"
#include "utils/StyleHelper.h"
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

    QPalette pal = palette();
    pal.setColor( QPalette::Window, QColor( "#454e59" ) );

    widget->setPalette( pal );
    widget->setAutoFillBackground( true );

    m_plInterface = Tomahawk::playlistinterface_ptr( new MetaArtistInfoInterface( this ) );

/*    TomahawkUtils::unmarginLayout( ui->layoutWidget->layout() );
    TomahawkUtils::unmarginLayout( ui->layoutWidget1->layout() );
    TomahawkUtils::unmarginLayout( ui->layoutWidget2->layout() );
    TomahawkUtils::unmarginLayout( ui->albumHeader->layout() );*/

    m_albumsModel = new PlayableModel( ui->albums );
    ui->albums->setPlayableModel( m_albumsModel );
    ui->topHits->setEmptyTip( tr( "Sorry, we could not find any albums for this artist!" ) );

    m_relatedModel = new PlayableModel( ui->relatedArtists );
    ui->relatedArtists->setPlayableModel( m_relatedModel );
    ui->relatedArtists->proxyModel()->sort( -1 );
    ui->topHits->setEmptyTip( tr( "Sorry, we could not find any related artists!" ) );

    m_topHitsModel = new PlaylistModel( ui->topHits );
    ui->topHits->proxyModel()->setStyle( PlayableProxyModel::Short );
    ui->topHits->setPlayableModel( m_topHitsModel );
    ui->topHits->setSortingEnabled( false );
    ui->topHits->setEmptyTip( tr( "Sorry, we could not find any top hits for this artist!" ) );

    ui->relatedArtists->setAutoFitItems( false );
    ui->relatedArtists->setWrapping( false );
    ui->relatedArtists->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    ui->relatedArtists->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    ui->relatedArtists->delegate()->setItemSize( QSize( 170, 170 ) );

    ui->albums->setAutoFitItems( false );
    ui->albums->setWrapping( false );
    ui->albums->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    ui->albums->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    ui->albums->delegate()->setItemSize( QSize( 170, 170 ) );
    ui->albums->proxyModel()->setHideDupeItems( true );

    ui->topHits->setFrameShape( QFrame::StyledPanel );
    ui->topHits->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    m_pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultArtistImage, TomahawkUtils::ScaledCover, QSize( 48, 48 ) );
    ui->cover->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultArtistImage, TomahawkUtils::ScaledCover, QSize( ui->cover->sizeHint() ) ) );

    ui->biography->setFrameShape( QFrame::NoFrame );
    ui->biography->setAttribute( Qt::WA_MacShowFocusRect, 0 );
    TomahawkUtils::styleScrollBar( ui->biography->verticalScrollBar() );

    QFont f = font();
    f.setBold( true );
    f.setPointSize( TomahawkUtils::defaultFontSize() + 5 );
    ui->artistLabel->setFont( f );

    QPalette p = ui->biography->palette();
    p.setColor( QPalette::Foreground, Qt::white );
    p.setColor( QPalette::Text, Qt::white );

    ui->artistLabel->setPalette( p );
    ui->biography->setPalette( p );
    ui->label->setPalette( p );
    ui->label_2->setPalette( p );
    ui->label_3->setPalette( p );

    QScrollArea* area = new QScrollArea();
    area->setWidgetResizable( true );
    area->setWidget( widget );

    area->setStyleSheet( "QScrollArea { background-color: #454e59; }" );
    area->setFrameShape( QFrame::NoFrame );
    area->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget( area );
    setLayout( layout );
    TomahawkUtils::unmarginLayout( layout );

    TomahawkUtils::styleScrollBar( ui->albums->horizontalScrollBar() );
    TomahawkUtils::styleScrollBar( ui->relatedArtists->horizontalScrollBar() );

    ui->biography->setStyleSheet( "QTextBrowser#biography { background-color: transparent; }" );

    ui->albums->setStyleSheet( "QListView { background-color: transparent; }" );
    ui->albumFrame->setStyleSheet( "QFrame#albumFrame { background-color: transparent; }"
                               "QFrame#albumFrame { "
                               "border-image: url(" RESPATH "images/scrollbar-vertical-handle.png) 3 3 3 3 stretch stretch;"
                               "border-top: 3px transparent; border-bottom: 3px transparent; border-right: 3px transparent; border-left: 3px transparent; }" );

    ui->relatedArtists->setStyleSheet( "QListView { background-color: transparent; }" );
    ui->artistFrame->setStyleSheet( "QFrame#artistFrame { background-color: transparent; }"
                               "QFrame#artistFrame { "
                               "border-image: url(" RESPATH "images/scrollbar-vertical-handle.png) 3 3 3 3 stretch stretch;"
                               "border-top: 3px transparent; border-bottom: 3px transparent; border-right: 3px transparent; border-left: 3px transparent; }" );

//    ui->topHits->setStyleSheet( "QTreeView#topHits { background-color: transparent; }" );
    ui->trackFrame->setStyleSheet( "QFrame#trackFrame { background-color: transparent; }"
                               "QFrame#trackFrame { "
                               "border-image: url(" RESPATH "images/scrollbar-vertical-handle.png) 3 3 3 3 stretch stretch;"
                               "border-top: 3px transparent; border-bottom: 3px transparent; border-right: 3px transparent; border-left: 3px transparent; }" );

    load( artist );
}


ArtistInfoWidget::~ArtistInfoWidget()
{
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
    if ( !m_artist.isNull() )
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

    connect( m_artist.data(), SIGNAL( biographyLoaded() ), SLOT( onBiographyLoaded() ) );
    connect( m_artist.data(), SIGNAL( similarArtistsLoaded() ), SLOT( onSimilarArtistsLoaded() ) );
    connect( m_artist.data(), SIGNAL( updated() ), SLOT( onArtistImageUpdated() ) );
    connect( m_artist.data(), SIGNAL( albumsAdded( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ),
                                SLOT( onAlbumsFound( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ) );
    connect( m_artist.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                                SLOT( onTracksFound( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode ) ) );

    ui->artistLabel->setText( artist->name() );

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
    m_topHitsModel->appendQueries( queries );
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

    ui->biography->setHtml( m_artist->biography() );
}


void
ArtistInfoWidget::onArtistImageUpdated()
{
    if ( m_artist->cover( QSize( 0, 0 ) ).isNull() )
        return;

    m_pixmap = m_artist->cover( QSize( 0, 0 ) );
    emit pixmapChanged( m_pixmap );

    ui->cover->setPixmap( m_artist->cover( ui->cover->sizeHint() ) );
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
