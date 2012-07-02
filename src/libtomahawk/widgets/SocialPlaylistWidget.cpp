/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Leo Franchi <lfranchi@kde.org>
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

#include "SocialPlaylistWidget.h"
#include "ui_SocialPlaylistWidget.h"

#include "database/DatabaseCommand_LoadDynamicPlaylist.h"
#include "database/Database.h"
#include "SourceList.h"
#include "PlayableModel.h"
#include "dynamic/GeneratorInterface.h"
#include "dynamic/database/DatabaseGenerator.h"
#include "utils/Logger.h"
#include "database/DatabaseCommand_GenericSelect.h"
#include "widgets/OverlayWidget.h"

using namespace Tomahawk;

QString SocialPlaylistWidget::s_popularAlbumsQuery = "SELECT * from album";
QString SocialPlaylistWidget::s_mostPlayedPlaylistsQuery = "asd";
QString SocialPlaylistWidget::s_topForeignTracksQuery = "select track.name, artist.name, count(*) as counter from (select track from playback_log group by track, source), track, artist where track not in (select track from playback_log where source is null group by track) and track.id = track and artist.id = track.artist group by track order by counter desc";

SocialPlaylistWidget::SocialPlaylistWidget ( QWidget* parent )
    : QWidget( parent )
    , ui( new Ui_SocialPlaylistWidget )
    , m_topForeignTracksModel( 0 )
    , m_popularNewAlbumsModel( 0 )
{
    ui->setupUi( this );

    ui->splitter_2->setStretchFactor( 0, 2 );
    ui->splitter_2->setStretchFactor( 0, 1 );

    /*
    WelcomePlaylistModel* model = new WelcomePlaylistModel( this );
    model->setMaxPlaylists( HISTORY_PLAYLIST_ITEMS );
    */

    ui->mostPlayedPlaylists->setFrameShape( QFrame::NoFrame );
    ui->mostPlayedPlaylists->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    TomahawkUtils::unmarginLayout( layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout->layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_2->layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_3->layout() );
    TomahawkUtils::unmarginLayout( ui->verticalLayout_4->layout() );

//     ui->mostPlayedPlaylists->setItemDelegate( new PlaylistDelegate() );
//     ui->mostPlayedPlaylists->setModel( model );
//     ui->mostPlayedPlaylists->overlay()->resize( 380, 86 );

//     connect( model, SIGNAL( emptinessChanged( bool) ), this, SLOT( updatePlaylists() ) );

    m_topForeignTracksModel = new PlaylistModel( ui->newTracksView );
    ui->newTracksView->setPlaylistModel( m_topForeignTracksModel );
    ui->newTracksView->proxyModel()->setStyle( PlayableProxyModel::Short );
    ui->newTracksView->overlay()->setEnabled( false );

    m_popularNewAlbumsModel = new PlayableModel( ui->newAlbumsView );
    ui->newAlbumsView->setPlayableModel( m_popularNewAlbumsModel );
    // TODO run the genericselect command
//     m_recentAlbumsModel->addFilteredCollection( collection_ptr(), 20, DatabaseCommand_AllAlbums::ModificationTime );
/*
    m_timer = new QTimer( this );
    connect( m_timer, SIGNAL( timeout() ), SLOT( checkQueries() ) );

    connect( SourceList::instance(), SIGNAL( ready() ), SLOT( updateRecentTracks() ) );
    connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), SLOT( onSourceAdded( Tomahawk::source_ptr ) ) );
    connect( ui->playlistWidget, SIGNAL( activated( QModelIndex ) ), SLOT( onPlaylistActivated( QModelIndex ) ) );
    connect( AudioEngine::instance() ,SIGNAL( playlistChanged( Tomahawk::playlistinterface_ptr ) ), this, SLOT( updatePlaylists() ), Qt::QueuedConnection );
*/
    fetchFromDB();
}


SocialPlaylistWidget::~SocialPlaylistWidget()
{
}


void
SocialPlaylistWidget::fetchFromDB()
{
    // Load the pre-baked custom playlists that we are going to show.
    QSharedPointer<DatabaseCommand_GenericSelect> albumsCmd = QSharedPointer<DatabaseCommand_GenericSelect>( new DatabaseCommand_GenericSelect( s_popularAlbumsQuery, DatabaseCommand_GenericSelect::Album, 30, 0 ) );
    connect( albumsCmd.data(), SIGNAL( albums( QList<Tomahawk::album_ptr> ) ), this, SLOT( popularAlbumsFetched( QList<Tomahawk::album_ptr> ) ) );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( albumsCmd ) );

//     QSharedPointer<DatabaseCommand_GenericSelect> plCmd = QSharedPointer<DatabaseCommand_GenericSelect>( new DatabaseCommand_GenericSelect( s_mostPlayedPlaylistsQuery, DatabaseCommand_GenericSelect::, 30, 0 ) );
//     connect( albumsCmd.data(), SIGNAL( albums( QList<Tomahawk::album_ptr> ) ), this, SLOT( popularAlbumsFetched( QList<Tomahawk::album_ptr> ) ) );
//     Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( albumsCmd ) );

    QSharedPointer<DatabaseCommand_GenericSelect> trackCmd = QSharedPointer<DatabaseCommand_GenericSelect>( new DatabaseCommand_GenericSelect( s_topForeignTracksQuery, DatabaseCommand_GenericSelect::Track, 50, 0 ) );
    connect( trackCmd.data(), SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( topForeignTracksFetched( QList<Tomahawk::query_ptr> ) ) );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( trackCmd ) );
}


Tomahawk::playlistinterface_ptr
SocialPlaylistWidget::playlistInterface() const
{
    return Tomahawk::playlistinterface_ptr();
}


/*
void
SocialPlaylistWidget::createPlaylist()
{
    // Ok, lets create our playlist

     * select count(*) as counter, track.name, artist.name from (select track from playback_log group by track, source), track, artist where track.id = track and artist.id = track.artist group by track order by counter desc limit 0,20;
     s elect count(*) as counter, playback_log.track, track.name, artist.name from playback_log, track, artist where track.id = playback_log.track and artist.id = track.artist group by playback_log.track order by counter desc limit 0,10;              *
     select count(*) as counter, track.name, artist.name from (select track from playback_log group by track, source), track, artist where track not in (select track from playback_log where source is null group by track) and track.id = track and artist.id = track.artist group by track order by counter desc limit 0,20;
     select count(*) as counter, track.name, artist.name from (select track from playback_log where source > 0 group by track, source), track, artist where track.id = track and artist.id = track.artist group by track order by counter desc limit 0,20;

    m_coolQuery1 = DynamicPlaylist::create( SourceList::instance()->getLocal(), COOLPLAYLIST_GUID, "Cool Playlist!", QString(), QString(), Static, false, "database", false );
    connect( m_coolQuery1.data(), SIGNAL( created() ), this, SLOT( playlist1Created() ) );
}*/


void
SocialPlaylistWidget::popularAlbumsFetched( QList< album_ptr > albums )
{
    m_popularNewAlbumsModel->clear();

    m_popularNewAlbumsModel->appendAlbums( albums );
}


void
SocialPlaylistWidget::topForeignTracksFetched( QList< query_ptr > tracks )
{
    m_topForeignTracksModel->clear();

    m_topForeignTracksModel->appendQueries( tracks );
}
