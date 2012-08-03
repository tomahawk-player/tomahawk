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

#include "AlbumInfoWidget.h"
#include "ui_AlbumInfoWidget.h"
#include "AlbumInfoWidget_p.h"

#include "audio/AudioEngine.h"
#include "ViewManager.h"
#include "database/Database.h"
#include "playlist/TreeModel.h"
#include "playlist/PlayableModel.h"
#include "Source.h"

#include "database/DatabaseCommand_AllTracks.h"
#include "database/DatabaseCommand_AllAlbums.h"

#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

using namespace Tomahawk;


AlbumInfoWidget::AlbumInfoWidget( const Tomahawk::album_ptr& album, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::AlbumInfoWidget )
{
    ui->setupUi( this );

    TomahawkUtils::unmarginLayout( layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_2 );

    m_albumsModel = new PlayableModel( ui->albumsView );
    ui->albumsView->setPlayableModel( m_albumsModel );
    ui->albumsView->setEmptyTip( tr( "Sorry, we could not find any other albums for this artist!" ) );
    ui->albumsView->proxyModel()->setHideDupeItems( true );

    m_tracksModel = new TreeModel( ui->tracksView );
    m_tracksModel->setMode( Mixed );
    ui->tracksView->setTreeModel( m_tracksModel );
    ui->tracksView->setRootIsDecorated( false );
    ui->tracksView->setEmptyTip( tr( "Sorry, we could not find any tracks for this album!" ) );
    ui->tracksView->proxyModel()->setStyle( PlayableProxyModel::Collection );

    m_pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultAlbumCover, TomahawkUtils::ScaledCover, QSize( 48, 48 ) );

    m_playlistInterface = playlistinterface_ptr( new MetaAlbumInfoInterface( this ) );
    load( album );
}


AlbumInfoWidget::~AlbumInfoWidget()
{
    delete ui;
}


Tomahawk::playlistinterface_ptr
AlbumInfoWidget::playlistInterface() const
{
    return m_playlistInterface;
}


bool
AlbumInfoWidget::isBeingPlayed() const
{
    //tDebug() << Q_FUNC_INFO << "audioengine playlistInterface = " << AudioEngine::instance()->currentTrackPlaylist()->id();
    //tDebug() << Q_FUNC_INFO << "albumsView playlistInterface = " << ui->albumsView->playlistInterface()->id();
    //tDebug() << Q_FUNC_INFO << "tracksView playlistInterface = " << ui->tracksView->playlistInterface()->id();

    if ( ui->albumsView && ui->albumsView->isBeingPlayed() )
        return true;

    if ( ui->albumsView && ui->albumsView->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    if ( ui->tracksView && ui->tracksView->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    return false;
}


bool
AlbumInfoWidget::jumpToCurrentTrack()
{
    return  ui->albumsView && ui->albumsView->jumpToCurrentTrack();
}


artist_ptr AlbumInfoWidget::descriptionArtist() const
{
    if ( !m_album.isNull() && !m_album->artist().isNull() )
        return m_album->artist();

    return artist_ptr();
}


ViewPage::DescriptionType
AlbumInfoWidget::descriptionType()
{
    if ( !m_album.isNull() && !m_album->artist().isNull() )
        return ViewPage::ArtistType;

    return ViewPage::TextType;
}


void
AlbumInfoWidget::load( const album_ptr& album )
{
    if ( !m_album.isNull() )
        disconnect( m_album.data(), SIGNAL( updated() ), this, SLOT( onAlbumCoverUpdated() ) );

    m_album = album;
    m_title = album->name();
    m_description = album->artist()->name();

    ui->albumsLabel->setText( tr( "Other Albums by %1" ).arg( album->artist()->name() ) );

    m_tracksModel->addTracks( album, QModelIndex(), true );
    loadAlbums( true );

    connect( m_album.data(), SIGNAL( updated() ), SLOT( onAlbumCoverUpdated() ) );
    onAlbumCoverUpdated();
}


void
AlbumInfoWidget::loadAlbums( bool autoRefetch )
{
    Q_UNUSED( autoRefetch );

    m_albumsModel->clear();

    connect( m_album->artist().data(), SIGNAL( albumsAdded( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ),
                                         SLOT( gotAlbums( QList<Tomahawk::album_ptr> ) ) );

    if ( !m_album->artist()->albums( Mixed ).isEmpty() )
        gotAlbums( m_album->artist()->albums( Mixed ) );

/*                tDebug() << "Auto refetching";
                m_buttonAlbums->setChecked( false );
                onAlbumsModeToggle();*/
}


void
AlbumInfoWidget::onAlbumCoverUpdated()
{
    if ( m_album->cover( QSize( 0, 0 ) ).isNull() )
        return;

    m_pixmap = m_album->cover( QSize( 0, 0 ) );
    emit pixmapChanged( m_pixmap );
}


void
AlbumInfoWidget::gotAlbums( const QList<Tomahawk::album_ptr>& albums )
{
    QList<Tomahawk::album_ptr> al = albums;
    if ( al.contains( m_album ) )
        al.removeAll( m_album );

    m_albumsModel->appendAlbums( al );
}


void
AlbumInfoWidget::changeEvent( QEvent* e )
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
