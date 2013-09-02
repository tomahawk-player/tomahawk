/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2011, Leo Franchi <lfranchi@kde.org>
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

#include "DatabaseCommand_LoadAllPlaylists_p.h"

#include "DatabaseImpl.h"
#include "Playlist.h"
#include "PlaylistEntry.h"
#include "Source.h"

#include <qjson/parser.h>

#include <QSqlQuery>

using namespace Tomahawk;


DatabaseCommand_LoadAllPlaylists::DatabaseCommand_LoadAllPlaylists( const source_ptr& s, QObject* parent )
    : DatabaseCommand( parent, new DatabaseCommand_LoadAllPlaylistsPrivate( this, s ) )
{
    qRegisterMetaType< QHash< Tomahawk::playlist_ptr, QStringList > >("QHash< Tomahawk::playlist_ptr, QStringList >");
}


void
DatabaseCommand_LoadAllPlaylists::exec( DatabaseImpl* dbi )
{
    Q_D( DatabaseCommand_LoadAllPlaylists );

    TomahawkSqlQuery query = dbi->newquery();
    QString orderToken, sourceToken;

    switch ( d->sortOrder )
    {
    case 0:
        break;

    case ModificationTime:
            orderToken = "ORDER BY playlist.createdOn";
    }

    if ( !source().isNull() )
        sourceToken = QString( "AND source %1 " ).arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) );

    QString trackIdJoin;
    QString trackIdFields;
    if ( d->returnPlEntryIds )
    {
        trackIdFields = ", pr.entries";
        trackIdJoin = "JOIN playlist_revision pr ON pr.playlist = p.guid AND pr.guid = p.currentrevision";
    }

    query.exec( QString( " SELECT p.guid, p.title, p.info, p.creator, p.lastmodified, p.shared, p.currentrevision, p.createdOn %6 "
                         " FROM playlist p "
                         " %5 "
                         " WHERE ( ( dynplaylist = 'false' ) OR ( dynplaylist = 0 ) ) "
                         " %1 "
                         " %2 %3 %4 "
                         )
                .arg( sourceToken )
                .arg( orderToken )
                .arg( d->sortDescending ? "DESC" : QString() )
                .arg( d->limitAmount > 0 ? QString( "LIMIT 0, %1" ).arg( d->limitAmount ) : QString() )
                .arg( trackIdJoin )
                .arg( trackIdFields )
                );

    QList<playlist_ptr> plists;
    QHash<playlist_ptr, QStringList> phash;
    QJson::Parser parser;
    while ( query.next() )
    {
        playlist_ptr p( new Playlist( source(),                  //src
                                      query.value(6).toString(), //current rev
                                      query.value(1).toString(), //title
                                      query.value(2).toString(), //info
                                      query.value(3).toString(), //creator
                                      query.value(7).toInt(),    //lastmod / createdOn
                                      query.value(5).toBool(),   //shared
                                      query.value(4).toInt(),    //lastmod
                                      query.value(0).toString()  //GUID
                                    ), &QObject::deleteLater );
        p->setWeakSelf( p.toWeakRef() );
        plists.append( p );

        if ( d->returnPlEntryIds )
        {
            QStringList trackIds = parser.parse( query.value( 8 ).toByteArray() ).toStringList();
            phash.insert( p, trackIds );
        }
    }

    emit done( plists );

    if ( d->returnPlEntryIds )
    {
        emit done( phash );
    }
}


void
DatabaseCommand_LoadAllPlaylists::setLimit( unsigned int limit )
{
    Q_D( DatabaseCommand_LoadAllPlaylists );
    d->limitAmount = limit;
}


void
DatabaseCommand_LoadAllPlaylists::setSortOrder( DatabaseCommand_LoadAllPlaylists::SortOrder order )
{
    Q_D( DatabaseCommand_LoadAllPlaylists );
    d->sortOrder = order;
}


void
DatabaseCommand_LoadAllPlaylists::setSortDescending( bool descending )
{
    Q_D( DatabaseCommand_LoadAllPlaylists );
    d->sortDescending = descending;
}


void
DatabaseCommand_LoadAllPlaylists::setReturnPlEntryIds( bool returnPlEntryIds )
{
    Q_D( DatabaseCommand_LoadAllPlaylists );
    d->returnPlEntryIds = returnPlEntryIds;
}

