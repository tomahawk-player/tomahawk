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

#include <QPushButton>
#include <QDialogButtonBox>

#include "SourceList.h"
#include "ViewManager.h"
#include "playlist/PlayableModel.h"
#include "playlist/PlaylistModel.h"
#include "utils/AnimatedSpinner.h"

#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"


SearchWidget::SearchWidget( const QString& search, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::SearchWidget )
    , m_search( search )
{
    ui->setupUi( this );

    ui->resultsView->setGuid( "searchwidget" );
    m_resultsModel = new PlaylistModel( ui->resultsView );
    ui->resultsView->setPlaylistModel( m_resultsModel );
    ui->resultsView->sortByColumn( PlaylistModel::Score, Qt::DescendingOrder );

    m_albumsModel = new PlayableModel( ui->albumView );
    ui->albumView->setPlayableModel( m_albumsModel );

    m_artistsModel = new PlayableModel( ui->artistView );
    ui->artistView->setPlayableModel( m_artistsModel );

    ui->artistView->proxyModel()->sort( -1 );
    ui->albumView->proxyModel()->sort( -1 );
    ui->artistView->proxyModel()->setHideDupeItems( true );
    ui->albumView->proxyModel()->setHideDupeItems( true );

    TomahawkUtils::unmarginLayout( ui->verticalLayout );

    m_artistsModel->startLoading();
    m_albumsModel->startLoading();
    m_resultsModel->startLoading();
    m_queries << Tomahawk::Query::get( search, uuid() );

    ui->splitter_2->setStretchFactor( 0, 0 );
    ui->splitter_2->setStretchFactor( 1, 1 );

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

        artists << result->artist();
        albums << result->album();
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
