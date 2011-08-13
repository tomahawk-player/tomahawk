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


#include "customplaylistview.h"

#include "database/databasecommand_genericselect.h"
#include "database/database.h"
#include "utils/tomahawkutils.h"
#include <sourcelist.h>

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

    setPlaylistModel( m_model );
    generateTracks();

    if ( m_type == SourceLovedTracks )
        connect( m_source.data(), SIGNAL( socialAttributesChanged() ), this, SLOT( reload() ) );
    else if ( m_type == AllLovedTracks )
    {
        connect( SourceList::instance()->getLocal().data(), SIGNAL( socialAttributesChanged() ), this, SLOT( reload() ) );
        foreach ( const source_ptr& s, SourceList::instance()->sources( true ) )
            connect( s.data(), SIGNAL( socialAttributesChanged() ), this, SLOT( reload() ) );

        connect( SourceList::instance(), SIGNAL( sourceAdded( Tomahawk::source_ptr ) ), this, SLOT( sourceAdded( Tomahawk::source_ptr ) ) );
    }
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
                           "ORDER BY counter DESC, social_attributes.timestamp DESC " ).arg( m_source->isLocal() ? "IS NULL" : QString( "=%1" ).arg( m_source->id() ) );
            break;
        case AllLovedTracks:
            sql = QString( "SELECT track.name, artist.name, source, COUNT(*) as counter "
                           "FROM social_attributes, track, artist "
                           "WHERE social_attributes.id = track.id AND artist.id = track.artist AND social_attributes.k = 'Love' AND social_attributes.v = 'true'"
                           "GROUP BY track.id "
                           "ORDER BY counter DESC, social_attributes.timestamp DESC " );
            break;
    }

    DatabaseCommand_GenericSelect* cmd = new DatabaseCommand_GenericSelect( sql, DatabaseCommand_GenericSelect::Track, -1, 0 );
    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksGenerated( QList<Tomahawk::query_ptr> ) ) );
    Database::instance()->enqueue( QSharedPointer<DatabaseCommand>( cmd ) );
}

void
CustomPlaylistView::tracksGenerated( QList< query_ptr > tracks )
{
    foreach ( const query_ptr& q, tracks )
        m_model->append( q );
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
CustomPlaylistView::reload()
{
    m_model->clear();
    generateTracks();
}


void
CustomPlaylistView::sourceAdded( const source_ptr& s)
{
    connect( s.data(), SIGNAL( socialAttributesChanged() ), this, SLOT( reload() ) );
}
