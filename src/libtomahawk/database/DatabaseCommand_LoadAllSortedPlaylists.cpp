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

#include "DatabaseCommand_LoadAllSortedPlaylists.h"
#include "DatabaseImpl.h"

#include "Playlist.h"
#include <libtomahawk/SourceList.h>

using namespace Tomahawk;

void
DatabaseCommand_LoadAllSortedPlaylists::exec( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();
    QString orderToken, sourceToken, ascDescToken;

    switch ( m_sortOrder )
    {
        case 0:
            break;

        case DatabaseCommand_LoadAllPlaylists::ModificationTime:
            orderToken = "playlist.createdOn";
    }

    switch ( m_sortAscDesc )
    {
        case DatabaseCommand_LoadAllPlaylists::NoOrder:
            break;
        case DatabaseCommand_LoadAllPlaylists::Ascending:
            ascDescToken = "ASC";
            break;
        case DatabaseCommand_LoadAllPlaylists::Descending:
            ascDescToken = "DESC";
            break;
    }

    if ( !source().isNull() )
        sourceToken = QString( "AND source %1 " ).arg( source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( source()->id() ) );


    query.exec( QString( "SELECT playlist.guid as guid, title, info, creator, lastmodified, shared, currentrevision, createdOn, dynplaylist, source, dynamic_playlist.pltype, dynamic_playlist.plmode "
                         "FROM playlist "
                         "LEFT JOIN dynamic_playlist ON playlist.guid = dynamic_playlist.guid  "
                         "%1 "
                         "%2 %3 %4"
    )
    .arg( sourceToken )
    .arg( m_sortOrder > 0 ? QString( "ORDER BY %1" ).arg( orderToken ) : QString() )
    .arg( ascDescToken )
    .arg( m_limitAmount > 0 ? QString( "LIMIT 0, %1" ).arg( m_limitAmount ) : QString() ) );

    QList<SourcePlaylistPair> plists;
    while ( query.next() )
    {
        plists << QPair< int, QString >( query.value(9).toInt(), query.value(0).toString() );

    }

    emit done( plists );
}
