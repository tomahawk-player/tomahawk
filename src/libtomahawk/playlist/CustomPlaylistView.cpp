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
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */


#include "CustomPlaylistView.h"

#include "database/DatabaseCommand_GenericSelect.h"
#include "database/Database.h"
#include "utils/TomahawkUtils.h"
#include "SourceList.h"
#include "audio/AudioEngine.h"

using namespace Tomahawk;

CustomPlaylistView::CustomPlaylistView( CustomPlaylistView::PlaylistType type, const source_ptr& s, QWidget* parent )
    : PlaylistView ( parent )
    , m_type( type )
    , m_source( s )
    , m_model( new PlaylistModel( this ) )
{
    // Generate the tracks, add them to the playlist
    setFrameShape( QFrame::NoFrame );
    setAttribute( Qt::WA_MacShowFocusRect, 0 );

    m_model->setStyle( TrackModel::Large );
    setPlaylistModel( m_model );
    generateTracks();

    if ( m_type == SourceLovedTracks )
        connect( m_source.data(), SIGNAL( socialAttributesChanged( QString ) ), this, SLOT( socialAttributesChanged( QString ) ) );
    else if ( m_type == TopLovedTracks )
    {
        connect( SourceList::instance()->getLocal().data(), SIGNAL( socialAttributesChanged( QString ) ), this, SLOT( socialAttributesChanged( QString ) ) );
        foreach ( const source_ptr& s, SourceList::instance()->sources( true ) )
            connect( s.data(), SIGNAL( socialAttributesChanged( QString ) ), this, SLOT( socialAttributesChanged( QString ) ) );

        connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), this, SLOT( sourceAdded( Tomahawk::source_ptr ) ) );
    }
}


CustomPlaylistView::~CustomPlaylistView()
{
}


bool
CustomPlaylistView::isBeingPlayed() const
{
    return AudioEngine::instance()->currentTrackPlaylist() == playlistInterface();
}


bool
CustomPlaylistView::jumpToCurrentTrack()
{
    return PlaylistView::jumpToCurrentTrack();
}


void
CustomPlaylistView::generateTracks()
{
    QString sql;
    switch ( m_type )
    {
        // TODO
        case SourceLovedTracks:
            sql = QString( "SELECT track.name, artist.name, COUNT(*) as counter "
                           "FROM social_attributes, track, artist "
                           "WHERE social_attributes.id = track.id AND artist.id = track.artist AND social_attributes.k = 'Love' AND social_attributes.v = 'true' AND social_attributes.source %1 "
                           "GROUP BY track.id "
                           "ORDER BY counter DESC, social_attributes.timestamp DESC " ).arg( m_source->isLocal() ? "IS NULL" : QString( "= %1" ).arg( m_source->id() ) );
            break;
        case TopLovedTracks:
            sql = QString( "SELECT track.name, artist.name, source, COUNT(*) as counter "
                           "FROM social_attributes, track, artist "
                           "WHERE social_attributes.id = track.id AND artist.id = track.artist AND social_attributes.k = 'Love' AND social_attributes.v = 'true' "
                           "GROUP BY track.id "
                           "ORDER BY counter DESC, social_attributes.timestamp DESC LIMIT 0, 50" );
            break;
    }

    DatabaseCommand_GenericSelect* cmd = new DatabaseCommand_GenericSelect( sql, DatabaseCommand_GenericSelect::Track, -1, 0 );
    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksGenerated( QList<Tomahawk::query_ptr> ) ) );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}


void
CustomPlaylistView::tracksGenerated( QList< query_ptr > tracks )
{
    bool changed = false;
    QList< query_ptr > newTracks = TomahawkUtils::mergePlaylistChanges( m_model->queries(), tracks, changed);

    if ( !changed )
        return;

    m_model->clear();
    m_model->append( newTracks );
}


QString
CustomPlaylistView::title() const
{
    if ( m_source.isNull() )
        return tr( "Top Loved Tracks" );
    else
    {
        if ( m_source->isLocal() )
            return tr( "Your loved tracks" );
        else
            return tr( "%1's loved tracks" ).arg( m_source->friendlyName() );
    }
}


QString
CustomPlaylistView::description() const
{
    if ( m_source.isNull() )
        return tr( "The most loved tracks from all your friends" );
    else
    {
        if ( m_source->isLocal() )
            return tr( "All of your loved tracks" );
        else
            return tr( "All of %1's loved tracks" ).arg( m_source->friendlyName() );
    }
}


QString
CustomPlaylistView::longDescription() const
{
    return QString();
}


QPixmap
CustomPlaylistView::pixmap() const
{
    return QPixmap( RESPATH "images/loved_playlist.png" );
}


void
CustomPlaylistView::socialAttributesChanged( const QString& action )
{
    if ( action == "Love" )
    {
        generateTracks();
    }
}


void
CustomPlaylistView::sourceAdded( const source_ptr& s )
{
    connect( s.data(), SIGNAL( socialAttributesChanged( QString ) ), this, SLOT( socialAttributesChanged( QString ) ) );
}
