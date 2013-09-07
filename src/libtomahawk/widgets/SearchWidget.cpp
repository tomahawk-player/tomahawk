/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "SearchWidget.h"
#include "ui_SearchWidget.h"

#include "SourceList.h"
#include "ViewManager.h"
#include "audio/AudioEngine.h"
#include "playlist/PlayableModel.h"
#include "playlist/PlaylistModel.h"
#include "utils/AnimatedSpinner.h"
#include "utils/TomahawkStyle.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

#include <QPushButton>
#include <QScrollArea>


SearchWidget::SearchWidget( const QString& search, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::SearchWidget )
    , m_search( search )
{
    QWidget* widget = new QWidget;
    ui->setupUi( widget );

    ui->lineAbove->setStyleSheet( QString( "QFrame { border: 1px solid black; }" ) );
    ui->lineBelow->setStyleSheet( QString( "QFrame { border: 1px solid %1; }" ).arg( TomahawkStyle::HEADER_BACKGROUND.name() ) );

    {
        ui->resultsView->setGuid( "searchwidget" );
        m_resultsModel = new PlaylistModel( ui->resultsView );

        QPalette p = ui->resultsView->palette();
        p.setColor( QPalette::Text, TomahawkStyle::PAGE_TRACKLIST_TRACK_SOLVED );
        p.setColor( QPalette::BrightText, TomahawkStyle::PAGE_TRACKLIST_TRACK_UNRESOLVED );
        p.setColor( QPalette::Foreground, TomahawkStyle::PAGE_TRACKLIST_NUMBER );
        p.setColor( QPalette::Highlight, TomahawkStyle::PAGE_TRACKLIST_HIGHLIGHT );
        p.setColor( QPalette::HighlightedText, TomahawkStyle::PAGE_TRACKLIST_HIGHLIGHT_TEXT );

        ui->resultsView->setPalette( p );
        TomahawkStyle::stylePageFrame( ui->resultsView );
        ui->resultsView->setFrameShape( QFrame::Panel );
        TomahawkStyle::stylePageFrame( ui->resultsFrame );

        ui->resultsView->setAlternatingRowColors( false );
        ui->resultsView->setAutoResize( true );
        ui->resultsView->setPlaylistModel( m_resultsModel );
        ui->resultsView->sortByColumn( PlaylistModel::Score, Qt::DescendingOrder );
        ui->resultsView->setEmptyTip( tr( "Sorry, we could not find any tracks!" ) );
    }

    {
        m_albumsModel = new PlayableModel( ui->albumView );
        ui->albumView->setPlayableModel( m_albumsModel );

        ui->albumView->proxyModel()->sort( -1 );
        ui->albumView->proxyModel()->setHideDupeItems( true );

        ui->albumView->setAutoResize( true );
        ui->albumView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        TomahawkStyle::stylePageFrame( ui->albumView );
        TomahawkStyle::stylePageFrame( ui->albumFrame );
    }

    {
        m_artistsModel = new PlayableModel( ui->artistView );
        ui->artistView->setPlayableModel( m_artistsModel );

        ui->artistView->proxyModel()->sort( -1 );
        ui->artistView->proxyModel()->setHideDupeItems( true );

        ui->artistView->setAutoResize( true );
        ui->artistView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        TomahawkStyle::stylePageFrame( ui->artistView );
        TomahawkStyle::stylePageFrame( ui->artistFrame );
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
        QFont f = ui->label_2->font();
        f.setFamily( "Pathway Gothic One" );

        QPalette p = ui->label_2->palette();
        p.setColor( QPalette::Foreground, TomahawkStyle::HEADER_TEXT );

        ui->label_2->setFont( f );
        ui->label_3->setFont( f );
        ui->label_2->setPalette( p );
        ui->label_3->setPalette( p );
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
        ui->resultsContainer->setPalette( pal );
        ui->resultsContainer->setAutoFillBackground( true );
    }

    m_artistsModel->startLoading();
    m_albumsModel->startLoading();
    m_resultsModel->startLoading();
    m_queries << Tomahawk::Query::get( search, uuid() );

    foreach ( const Tomahawk::query_ptr& query, m_queries )
    {
        connect( query.data(), SIGNAL( artistsAdded( QList<Tomahawk::artist_ptr> ) ), SLOT( onArtistsFound( QList<Tomahawk::artist_ptr> ) ) );
        connect( query.data(), SIGNAL( albumsAdded( QList<Tomahawk::album_ptr> ) ), SLOT( onAlbumsFound( QList<Tomahawk::album_ptr> ) ) );
        connect( query.data(), SIGNAL( resultsAdded( QList<Tomahawk::result_ptr> ) ), SLOT( onResultsFound( QList<Tomahawk::result_ptr> ) ) );
        connect( query.data(), SIGNAL( resolvingFinished( bool ) ), SLOT( onQueryFinished() ) );
    }
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
    return ui->resultsView->playlistInterface();
}


bool
SearchWidget::jumpToCurrentTrack()
{
    return ui->resultsView->jumpToCurrentTrack();
}


void
SearchWidget::onResultsFound( const QList<Tomahawk::result_ptr>& results )
{
    QList<Tomahawk::artist_ptr> artists;
    QList<Tomahawk::album_ptr> albums;
    foreach( const Tomahawk::result_ptr& result, results )
    {
        if ( !result->collection().isNull() && !result->isOnline() )
            continue;

        QList< Tomahawk::result_ptr > rl;
        rl << result;

        Tomahawk::query_ptr q = result->toQuery();
        q->addResults( rl );

        m_resultsModel->appendQuery( q );

        artists << result->track()->artistPtr();
        albums << result->track()->albumPtr();
    }

    onArtistsFound( artists );
    onAlbumsFound( albums );
}


void
SearchWidget::onAlbumsFound( const QList<Tomahawk::album_ptr>& albums )
{
    foreach ( const Tomahawk::album_ptr& album, albums )
    {
        int distance = TomahawkUtils::levenshtein( m_search, album->name() );
        int maxlen = qMax( m_search.length(), album->name().length() );
        float score = (float)( maxlen - distance ) / maxlen;

        if ( score <= 0.1 )
            continue;

        m_albums.insert( score, album );
//        tDebug() << Q_FUNC_INFO << "found album:" << album->name() << "score:" << score;
    }

    updateAlbums();
}


void
SearchWidget::onArtistsFound( const QList<Tomahawk::artist_ptr>& artists )
{
    foreach ( const Tomahawk::artist_ptr& artist, artists )
    {
        int distance = TomahawkUtils::levenshtein( m_search, artist->name() );
        int maxlen = qMax( m_search.length(), artist->name().length() );
        float score = (float)( maxlen - distance ) / maxlen;

        if ( score <= 0.1 )
            continue;

        m_artists.insert( score, artist );
//        tDebug() << Q_FUNC_INFO << "found artist:" << artist->name() << "score:" << score;
    }

    updateArtists();
}


void
SearchWidget::onQueryFinished()
{
    m_artistsModel->finishLoading();
    m_albumsModel->finishLoading();
    m_resultsModel->finishLoading();
}


void
SearchWidget::updateArtists()
{
    QList< Tomahawk::artist_ptr > sortedArtists;
    QList< float > floats = m_artists.keys();
    qSort( floats.begin(), floats.end() );

    for ( int i = floats.count() - 1; i >= 0; i-- )
    {
        sortedArtists << m_artists.value( floats.at( i ) );
    }

    m_artistsModel->clear();
    m_artistsModel->appendArtists( sortedArtists );
}


void
SearchWidget::updateAlbums()
{
    QList< Tomahawk::album_ptr > sortedAlbums;
    QList< float > floats = m_albums.keys();
    qSort( floats.begin(), floats.end() );

    for ( int i = floats.count() - 1; i >= 0; i-- )
    {
        sortedAlbums << m_albums.value( floats.at( i ) );
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
    if ( ui->resultsView->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;
    if ( ui->resultsView->playlistInterface()->hasChildInterface( AudioEngine::instance()->currentTrackPlaylist() ) )
        return true;

    if ( ui->albumView->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;
    if ( ui->albumView->playlistInterface()->hasChildInterface( AudioEngine::instance()->currentTrackPlaylist() ) )
        return true;

    if ( ui->artistView->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;
    if ( ui->artistView->playlistInterface()->hasChildInterface( AudioEngine::instance()->currentTrackPlaylist() ) )
        return true;

    return false;
}
