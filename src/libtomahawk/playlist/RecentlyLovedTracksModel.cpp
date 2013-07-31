/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2013, Uwe L. Korn <uwelk@xhochy.com>
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

#include "RecentlyLovedTracksModel_p.h"

#include "database/Database.h"
#include "database/DatabaseCommand_GenericSelect.h"
#include "Source.h"

RecentlyLovedTracksModel::RecentlyLovedTracksModel( QObject* parent )
    : LovedTracksModel( parent, new RecentlyLovedTracksModelPrivate( this ) )
{
}


RecentlyLovedTracksModel::~RecentlyLovedTracksModel()
{
}


void
RecentlyLovedTracksModel::loadTracks()
{
    Q_D( RecentlyLovedTracksModel );
    startLoading();

    QString sql;
    if ( d->source.isNull() )
    {
        sql = QString( "SELECT track.name, artist.name, source "
                       "FROM social_attributes, track, artist "
                       "WHERE social_attributes.id = track.id AND artist.id = track.artist AND social_attributes.k = 'Love' AND social_attributes.v = 'true' "
                       "ORDER BY social_attributes.timestamp DESC LIMIT %1" )
                .arg( d->limit );
    }
    else
    {
        sql = QString( "SELECT track.name, artist.name "
                       "FROM social_attributes, track, artist "
                       "WHERE social_attributes.id = track.id AND artist.id = track.artist AND social_attributes.k = 'Love' AND social_attributes.v = 'true' AND social_attributes.source %1 "
                       "ORDER BY social_attributes.timestamp DESC "
                       )
                .arg( d->source->isLocal() ? "IS NULL" : QString( "= %1" ).arg( d->source->id() ) );
    }

    Tomahawk::DatabaseCommand_GenericSelect* cmd = new Tomahawk::DatabaseCommand_GenericSelect( sql, Tomahawk::DatabaseCommand_GenericSelect::Track, -1, 0 );
    connect( cmd, SIGNAL( tracks( QList<Tomahawk::query_ptr> ) ), this, SLOT( tracksLoaded( QList<Tomahawk::query_ptr> ) ) );
    Tomahawk::Database::instance()->enqueue( Tomahawk::dbcmd_ptr( cmd ) );
}
