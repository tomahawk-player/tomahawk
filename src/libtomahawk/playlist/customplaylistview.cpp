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
                           "WHERE social_attributes.id = track.id AND artist.id = track.artist AND social_attributes.k = 'Love' AND source IS NULL "
                           "GROUP BY track.id "
                           "ORDER BY counter DESC " );
            break;
        case AllLovedTracks:
            sql = QString( "SELECT track.name, artist.name, source, COUNT(*) as counter "
                           "FROM social_attributes, track, artist "
                           "WHERE social_attributes.id = track.id AND artist.id = track.artist AND social_attributes.k = 'Love' "
                           "GROUP BY track.id "
                           "ORDER BY counter DESC " );
            break;
    }

    DatabaseCommand_GenericSelect* cmd = new DatabaseCommand_GenericSelect( sql, DatabaseCommand_GenericSelect::Track, 30, 0 );
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
        return tr( "Your Loved Tracks" );
}


QString
CustomPlaylistView::description() const
{
    if ( m_source.isNull() )
        return tr( "The most loved tracks from all your friends" );
    else
        return tr( "Your top loved tracks" );
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
