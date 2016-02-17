/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2015, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2012       Leo Franchi            <lfranchi@kde.org>
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

#include "SearchViewPage.h"
#include "ui_SearchViewPage.h"

#include "SourceList.h"
#include "MetaPlaylistInterface.h"
#include "ViewManager.h"
#include "audio/AudioEngine.h"
#include "playlist/ContextView.h"
#include "playlist/TrackView.h"
#include "playlist/PlayableModel.h"
#include "playlist/PlaylistModel.h"
#include "playlist/GridItemDelegate.h"
#include "utils/AnimatedSpinner.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"
#include "widgets/BasicHeader.h"

#include <QPushButton>
#include <QScrollArea>

using namespace Tomahawk;


SearchWidget::SearchWidget( const QString& search, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::SearchWidget )
    , m_search( search )
{
    QWidget* widget = new QWidget;
    BasicHeader* headerWidget = new BasicHeader;
    ui->setupUi( widget );

    {
        ui->artists->setAutoResize( true );
        ui->artists->setAutoFitItems( false );
        ui->artists->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        ui->artists->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        ui->artists->setWrapping( false );
        ui->artists->setItemWidth( TomahawkUtils::DpiScaler::scaledX( this, 140 ) );
        ui->artists->setFixedHeight( ui->artists->itemSize().height() + ui->artists->spacing() * 2 );

        m_artistsModel = new PlayableModel( ui->artists );
        ui->artists->setPlayableModel( m_artistsModel );
        ui->artists->proxyModel()->sort( -1 );
        ui->artists->setEmptyTip( tr( "Sorry, we could not find any artists!" ) );

        TomahawkStyle::stylePageFrame( ui->artists );
        TomahawkStyle::stylePageFrame( ui->artistFrame );
        TomahawkStyle::styleScrollBar( ui->artists->verticalScrollBar() );
    }

    {
        ui->albums->setAutoResize( true );
        ui->albums->setAutoFitItems( false );
        ui->albums->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        ui->albums->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        ui->albums->setWrapping( false );
        ui->albums->setItemWidth( TomahawkUtils::DpiScaler::scaledX( this, 140 ) );
//        ui->albums->proxyModel()->setHideDupeItems( true );
        ui->albums->delegate()->setWordWrapping( true );
        ui->albums->setFixedHeight( ui->albums->itemSize().height() + ui->albums->spacing() * 2 );

        m_albumsModel = new PlayableModel( ui->albums );
        ui->albums->setPlayableModel( m_albumsModel );
        ui->albums->proxyModel()->sort( -1 );
        ui->albums->setEmptyTip( tr( "Sorry, we could not find any albums!" ) );

        ui->albums->setStyleSheet( QString( "QListView { background-color: %1; }" ).arg( TomahawkStyle::PAGE_BACKGROUND.name() ) );
        TomahawkStyle::stylePageFrame( ui->albumFrame );
        TomahawkStyle::styleScrollBar( ui->albums->verticalScrollBar() );
        TomahawkStyle::styleScrollBar( ui->albums->horizontalScrollBar() );
    }

    {
        ui->tracks->setAutoResize( true );
        ui->tracks->setAutoFitItems( false );
        ui->tracks->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        ui->tracks->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        ui->tracks->setWrapping( false );
        ui->tracks->setItemWidth( TomahawkUtils::DpiScaler::scaledX( this, 140 ) );
//        ui->tracks->proxyModel()->setHideDupeItems( true );
        ui->tracks->delegate()->setWordWrapping( true );
        ui->tracks->delegate()->setShowBuyButtons( true );
        ui->tracks->setFixedHeight( ui->tracks->itemSize().height() + ui->tracks->spacing() * 2 );

        m_resultsModel = new PlayableModel( ui->tracks );
        ui->tracks->setPlayableModel( m_resultsModel );
        ui->tracks->proxyModel()->sort( -1 );
        ui->tracks->setEmptyTip( tr( "Sorry, we could not find any songs!" ) );

        ui->tracks->setStyleSheet( QString( "QListView { background-color: %1; }" ).arg( TomahawkStyle::PAGE_BACKGROUND.name() ) );
        TomahawkStyle::stylePageFrame( ui->trackFrame );
    }

    {
        QFont f = ui->topHitsMoreLabel->font();
        f.setWeight( QFont::Light );
        f.setPointSize( TomahawkUtils::defaultFontSize() + 1 );
        ui->topHitsMoreLabel->setFont( f );
        ui->artistsMoreLabel->setFont( f );
        ui->albumsMoreLabel->setFont( f );

        connect( ui->artistsMoreLabel, SIGNAL( clicked() ), SLOT( onArtistsMoreClicked() ) );
        connect( ui->albumsMoreLabel, SIGNAL( clicked() ), SLOT( onAlbumsMoreClicked() ) );
        connect( ui->topHitsMoreLabel, SIGNAL( clicked() ), SLOT( onTopHitsMoreClicked() ) );
    }

    m_stackedWidget = new QStackedWidget();

    {
        QScrollArea* area = new QScrollArea();
        area->setWidgetResizable( true );
        area->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
        area->setWidget( widget );

        QPalette pal = palette();
        pal.setBrush( backgroundRole(), TomahawkStyle::PAGE_BACKGROUND );
        area->setPalette( pal );
        area->setAutoFillBackground( true );
        area->setFrameShape( QFrame::NoFrame );
        area->setAttribute( Qt::WA_MacShowFocusRect, 0 );

        m_stackedWidget->addWidget( area );
    }
    {
        ContextView* topHitsFullView = new ContextView( m_stackedWidget );
        topHitsFullView->setCaption( tr( "Songs" ) );
        topHitsFullView->setShowCloseButton( true );
        topHitsFullView->trackView()->setPlayableModel( m_resultsModel );
        m_stackedWidget->addWidget( topHitsFullView );

        connect( topHitsFullView, SIGNAL( closeClicked() ), SLOT( onTopHitsMoreClosed() ) );
    }
    {
        GridView* artistsFullView = new GridView();
        artistsFullView->setPlayableModel( m_artistsModel );

        CaptionLabel* captionLabel = new CaptionLabel( this );
        captionLabel->setText( tr( "Artists" ) );
        captionLabel->setShowCloseButton( true );

        QWidget* vbox = new QWidget;
        QPalette pal = vbox->palette();
        pal.setBrush( vbox->backgroundRole(), TomahawkStyle::PAGE_BACKGROUND );
        vbox->setPalette( pal );
        vbox->setAutoFillBackground( true );

        QVBoxLayout* vboxl = new QVBoxLayout;
        TomahawkUtils::unmarginLayout( vboxl );
        vboxl->setContentsMargins( 32, 32, 32, 32 );
        vboxl->setSpacing( 8 );
        vbox->setLayout( vboxl );

        vboxl->addWidget( captionLabel );
        vboxl->addWidget( artistsFullView );
        vboxl->addStretch();
        vboxl->setStretchFactor( artistsFullView, 1 );

        m_stackedWidget->addWidget( vbox );

        connect( captionLabel, SIGNAL( clicked() ), SLOT( onTopHitsMoreClosed() ) );
    }
    {
        GridView* albumsFullView = new GridView( m_stackedWidget );
        albumsFullView->setPlayableModel( m_albumsModel );
        albumsFullView->delegate()->setWordWrapping( true );

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
        vboxl->setSpacing( 8 );
        vbox->setLayout( vboxl );

        vboxl->addWidget( captionLabel );
        vboxl->addWidget( albumsFullView );
        vboxl->addStretch();
        vboxl->setStretchFactor( albumsFullView, 1 );

        m_stackedWidget->addWidget( vbox );

        connect( captionLabel, SIGNAL( clicked() ), SLOT( onTopHitsMoreClosed() ) );
    }

    {
        QVBoxLayout* layout = new QVBoxLayout();
        layout->addWidget( headerWidget );
        layout->addWidget( m_stackedWidget );
        setLayout( layout );
        TomahawkUtils::unmarginLayout( layout );
    }

    MetaPlaylistInterface* mpl = new MetaPlaylistInterface();
    mpl->addChildInterface( ui->artists->playlistInterface() );
    mpl->addChildInterface( ui->tracks->playlistInterface() );
    mpl->addChildInterface( ui->albums->playlistInterface() );
    m_plInterface = playlistinterface_ptr( mpl );

    headerWidget->setCaption( title() );

    m_artistsModel->startLoading();
    m_albumsModel->startLoading();
    m_resultsModel->startLoading();

    m_query = Tomahawk::Query::get( search, uuid() );
    connect( m_query.data(), SIGNAL( artistsAdded( QList<Tomahawk::artist_ptr> ) ), SLOT( onArtistsFound( QList<Tomahawk::artist_ptr> ) ) );
    connect( m_query.data(), SIGNAL( albumsAdded( QList<Tomahawk::album_ptr> ) ), SLOT( onAlbumsFound( QList<Tomahawk::album_ptr> ) ) );
    connect( m_query.data(), SIGNAL( resultsAdded( QList<Tomahawk::result_ptr> ) ), SLOT( onResultsFound( QList<Tomahawk::result_ptr> ) ) );
    connect( m_query.data(), SIGNAL( resolvingFinished( bool ) ), SLOT( onQueryFinished() ) );

    TomahawkUtils::fixMargins( this );
}


SearchWidget::~SearchWidget()
{
    tDebug() << Q_FUNC_INFO;

    delete ui;
}


void
SearchWidget::changeEvent( QEvent* e )
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


Tomahawk::playlistinterface_ptr
SearchWidget::playlistInterface() const
{
    return m_plInterface;
}


bool
SearchWidget::jumpToCurrentTrack()
{
    if ( ui->albums && ui->albums->jumpToCurrentTrack() )
        return true;

    if ( ui->artists && ui->artists->jumpToCurrentTrack() )
        return true;

    if ( ui->tracks && ui->tracks->jumpToCurrentTrack() )
        return true;

    return false;
}


void
SearchWidget::onResultsFound( const QList<Tomahawk::result_ptr>& results )
{
    tDebug() << Q_FUNC_INFO;

    QList<Tomahawk::artist_ptr> artists;
    QList<Tomahawk::album_ptr> albums;
    QList<Tomahawk::query_ptr> queries;

    foreach( const Tomahawk::result_ptr& result, results )
    {
        if ( !result->resolvedByCollection().isNull() && !result->isOnline() )
            continue;

        QList< Tomahawk::result_ptr > rl;
        rl << result;

        Tomahawk::query_ptr query = result->toQuery();
        query->disallowReresolve();

        bool found = false;
        foreach ( const Tomahawk::query_ptr& q, m_results.keys() )
        {
            if ( q->equals( query, true, true ) )
            {
                found = true;
                q->addResults( rl );
                break;
            }
        }

        if ( !found )
        {
            m_results.insert( query, /*FIXME*/ query->score() );
            queries << query;
        }

        artists << result->track()->artistPtr();
        albums << result->track()->albumPtr();
    }

    while ( queries.count() )
    {
        query_ptr q = queries.takeFirst();
        if ( q->results().isEmpty() )
            continue;

        bool done = false;
        for ( int i = 0; i < m_resultsModel->rowCount( QModelIndex() ); i++ )
        {
            PlayableItem* item = m_resultsModel->itemFromIndex( m_resultsModel->index( i, 0, QModelIndex() ) );
            if ( item && item->query() && item->query()->numResults( true ) )
            {
                if ( m_query->howSimilar( item->query()->results().first() ) < m_query->howSimilar( q->results().first() ) )
                {
                    m_resultsModel->insertQuery( q, i );
                    done = true;
                    break;
                }
            }
        }

        if ( !done )
        {
            m_resultsModel->appendQuery( q );
        }
    }

    onArtistsFound( artists );
    onAlbumsFound( albums );
}


void
SearchWidget::onAlbumsFound( const QList<Tomahawk::album_ptr>& albums )
{
    tDebug() << Q_FUNC_INFO;

    foreach ( const Tomahawk::album_ptr& album, albums )
    {
        if ( m_albums.contains( album ) )
            continue;

        int distance = TomahawkUtils::levenshtein( m_search, album->name() );
        int maxlen = qMax( m_search.length(), album->name().length() );
        float scoreAlbum = (float)( maxlen - distance ) / maxlen;

        distance = TomahawkUtils::levenshtein( m_search, album->artist()->name() );
        maxlen = qMax( m_search.length(), album->artist()->name().length() );
        float scoreArtist = (float)( maxlen - distance ) / maxlen;

        float scoreMax = qMax( scoreAlbum, scoreArtist );
        if ( scoreMax <= 0.1 )
            continue;

        m_albums.insert( album, scoreMax );
//        tDebug() << Q_FUNC_INFO << "found album:" << album->name() << "scoreMax:" << scoreMax;
    }

//    updateAlbums();
}


void
SearchWidget::onArtistsFound( const QList<Tomahawk::artist_ptr>& artists )
{
    tDebug() << Q_FUNC_INFO;

    foreach ( const Tomahawk::artist_ptr& artist, artists )
    {
        if ( m_artists.contains( artist ) )
            continue;

        int distance = TomahawkUtils::levenshtein( m_search, artist->name() );
        int maxlen = qMax( m_search.length(), artist->name().length() );
        float score = (float)( maxlen - distance ) / maxlen;

        if ( score <= 0.1 )
            continue;

        m_artists.insert( artist, score );
//        tDebug() << Q_FUNC_INFO << "found artist:" << artist->name() << "score:" << score;
    }

//    updateArtists();
}


void
SearchWidget::onQueryFinished()
{
    tDebug() << Q_FUNC_INFO;

    updateArtists();
    updateAlbums();

    m_artistsModel->finishLoading();
    m_albumsModel->finishLoading();
    m_resultsModel->finishLoading();
}


void
SearchWidget::updateArtists()
{
    tDebug() << Q_FUNC_INFO;

    QList< Tomahawk::artist_ptr > sortedArtists;
    QList< artist_ptr > artists = m_artists.keys();
    QList< float > floats = m_artists.values();

    qSort( floats.begin(), floats.end() );

    while ( !floats.isEmpty() )
    {
        float f = floats.takeLast();
        foreach ( const artist_ptr& a, artists )
        {
            if ( m_artists.value( a ) == f )
            {
                artists.removeAll( a );
                sortedArtists << a;
                break;
            }
        }
    }

    m_artistsModel->clear();
    m_artistsModel->appendArtists( sortedArtists );
}


void
SearchWidget::updateAlbums()
{
    tDebug() << Q_FUNC_INFO;

    QList< Tomahawk::album_ptr > sortedAlbums;
    QList< album_ptr > albums = m_albums.keys();
    QList< float > floats = m_albums.values();

    qSort( floats.begin(), floats.end() );

    while ( !floats.isEmpty() )
    {
        float f = floats.takeLast();
        foreach ( const album_ptr& a, albums )
        {
            if ( m_albums.value( a ) == f )
            {
                albums.removeAll( a );
                sortedAlbums << a;
                break;
            }
        }
    }

    m_albumsModel->clear();
    m_albumsModel->appendAlbums( sortedAlbums );
}


QPixmap
SearchWidget::pixmap() const
{
    return TomahawkUtils::defaultPixmap( TomahawkUtils::Search );
}


bool
SearchWidget::isBeingPlayed() const
{
    if ( ui->albums && ui->albums->isBeingPlayed() )
        return true;

    if ( ui->artists && ui->artists->isBeingPlayed() )
        return true;

    if ( ui->albums && ui->albums->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    if ( ui->artists && ui->artists->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    if ( ui->tracks && ui->tracks->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    return false;
}


void
SearchWidget::onTopHitsMoreClicked()
{
    m_stackedWidget->setCurrentIndex( 1 );
}


void
SearchWidget::onArtistsMoreClicked()
{
    m_stackedWidget->setCurrentIndex( 2 );
}


void
SearchWidget::onAlbumsMoreClicked()
{
    m_stackedWidget->setCurrentIndex( 3 );
}


void
SearchWidget::onTopHitsMoreClosed()
{
    m_stackedWidget->setCurrentIndex( 0 );
}
