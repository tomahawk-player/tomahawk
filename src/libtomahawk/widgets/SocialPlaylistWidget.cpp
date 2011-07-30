/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
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

#include "SocialPlaylistWidget.h"
#include "ui_SocialPlaylistWidget.h"

#include "database/databasecommand_loaddynamicplaylist.h"
#include "database/database.h"
#include "sourcelist.h"
#include "dynamic/GeneratorInterface.h"
#include "dynamic/database/DatabaseGenerator.h"
#include "utils/logger.h"

#define COOLPLAYLIST_GUID "TOMAHAWK_COOLPLAYLISTOHAI_GUID"

using namespace Tomahawk;

SocialPlaylistWidget::SocialPlaylistWidget ( QWidget* parent )
    : QWidget ( parent )
    , ui( new Ui_SocialPlaylistWidget )
    , m_coolQuery1Model( new PlaylistModel( this )  )
{
    ui->setupUi( this );

    ui->playlistView->setPlaylistModel( m_coolQuery1Model );

    load();
}

SocialPlaylistWidget::~SocialPlaylistWidget()
{

}

void
SocialPlaylistWidget::load()
{
    // Load the pre-baked custom playlists that we are going to show.
    // They also might not exist yet if this the first time this is shown, so then we create them.
    DatabaseCommand_LoadDynamicPlaylist* cmd = new DatabaseCommand_LoadDynamicPlaylist( SourceList::instance()->getLocal(), COOLPLAYLIST_GUID, 0 );
    connect( cmd, SIGNAL( dynamicPlaylistLoaded( Tomahawk::dynplaylist_ptr ) ), this, SLOT( dynamicPlaylistLoaded( Tomahawk::dynplaylist_ptr ) ) );
    connect( cmd, SIGNAL( done() ), this, SLOT( dynamicPlaylistLoadDone() ) );

    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}

PlaylistInterface*
SocialPlaylistWidget::playlistInterface() const
{
    return ui->playlistView->proxyModel();
}

void
SocialPlaylistWidget::dynamicPlaylistLoaded ( const dynplaylist_ptr& ptr )
{
    m_coolQuery1 = ptr;
    tLog() << "SocialPlaylistWidget got dynplaylist loaded with currev: " << m_coolQuery1->currentrevision();
    connect( m_coolQuery1.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ), this, SLOT( playlistRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ) );
    connect( m_coolQuery1->generator().data(), SIGNAL(generated( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksGenerated( QList<Tomahawk::query_ptr> ) ) );

    connect( m_coolQuery1->generator().data(), SIGNAL(generated( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksGenerated( QList<Tomahawk::query_ptr> ) ) );
    m_coolQuery1->loadRevision( m_coolQuery1->currentrevision() );
}

void
SocialPlaylistWidget::playlistRevisionLoaded( Tomahawk::DynamicPlaylistRevision )
{
    m_coolQuery1Model->loadPlaylist( m_coolQuery1 );
    m_coolQuery1->resolve();

    if ( m_coolQuery1->entries().isEmpty() ) // Generate some
        m_coolQuery1->generator()->generate( 30 );

}

void
SocialPlaylistWidget::dynamicPlaylistLoadDone()
{
    if ( m_coolQuery1.isNull() ) /// Load failed so we need to create the playlist, doesn't exist yet
    {
        tLog() << "SocialPlaylistWidget didn't find the magic dynamic playlist, creating!";
        createPlaylist();
    }
}

void
SocialPlaylistWidget::createPlaylist()
{
    // Ok, lets create our playlist
    /**
     * select count(*) as counter, track.name, artist.name from (select track from playback_log group by track, source), track, artist where track.id = track and artist.id = track.artist group by track order by counter desc limit 0,20;
     s elect count(*) as counter, playback_log.track, track.name, artist.name from playback_log, track, artist where track.id = playback_log.track and artist.id = track.artist group by playback_log.track order by counter desc limit 0,10;              *
     select count(*) as counter, track.name, artist.name from (select track from playback_log group by track, source), track, artist where track not in (select track from playback_log where source is null group by track) and track.id = track and artist.id = track.artist group by track order by counter desc limit 0,20;
     select count(*) as counter, track.name, artist.name from (select track from playback_log where source > 0 group by track, source), track, artist where track.id = track and artist.id = track.artist group by track order by counter desc limit 0,20;
     */
    m_coolQuery1 = DynamicPlaylist::create( SourceList::instance()->getLocal(), COOLPLAYLIST_GUID, "Cool Playlist!", QString(), QString(), Static, false, "database", false );
    connect( m_coolQuery1.data(), SIGNAL( created() ), this, SLOT( playlist1Created() ) );
}

void
SocialPlaylistWidget::playlist1Created()
{
    Q_ASSERT( m_coolQuery1->generator().dynamicCast< DatabaseGenerator >() );

    QString sql = "select track.name, artist.name, count(*) as counter from (select track from playback_log group by track, source), track, artist where track.id = track and artist.id = track.artist group by track order by counter desc limit 0,100;";

    dyncontrol_ptr control = m_coolQuery1->generator().dynamicCast< DatabaseGenerator >()->createControl( sql, "This is a cool playlist!" );
    m_coolQuery1->createNewRevision( uuid() );

    connect( m_coolQuery1.data(), SIGNAL( dynamicRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ), this, SLOT( playlistRevisionLoaded( Tomahawk::DynamicPlaylistRevision ) ) );
    connect( m_coolQuery1->generator().data(), SIGNAL(generated( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksGenerated( QList<Tomahawk::query_ptr> ) ) );
}

void
SocialPlaylistWidget::tracksGenerated( QList< query_ptr > queries )
{
    if ( sender() == m_coolQuery1->generator().data() )
    {
        tDebug() << "Got generated tracks from playlist, adding" << queries.size() << "tracks";
        m_coolQuery1Model->clear();
        m_coolQuery1->addEntries( queries, m_coolQuery1->currentrevision() );
    }
}
