/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
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

#include "databasecommand_allartists.h"

#include <QSqlQuery>

#include "databaseimpl.h"
#include "utils/logger.h"


void
DatabaseCommand_AllArtists::exec( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();
    QList<Tomahawk::artist_ptr> al;
    QString orderToken, sourceToken;

    switch ( m_sortOrder )
    {
        case 0:
            break;

        case ModificationTime:
            orderToken = "file.mtime";
    }

    if ( !m_collection.isNull() )
        sourceToken = QString( "AND file.source %1 " ).arg( m_collection->source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( m_collection->source()->id() ) );

    QString sql = QString(
            "SELECT DISTINCT artist.id, artist.name "
            "FROM artist, file, file_join "
            "WHERE file.id = file_join.file "
            "AND file_join.artist = artist.id "
            "%1 %2 %3 %4"
            ).arg( sourceToken )
             .arg( m_sortOrder > 0 ? QString( "ORDER BY %1" ).arg( orderToken ) : QString() )
             .arg( m_sortDescending ? "DESC" : QString() )
             .arg( m_amount > 0 ? QString( "LIMIT 0, %1" ).arg( m_amount ) : QString() );

    query.prepare( sql );
    query.exec();

    while( query.next() )
    {
        Tomahawk::artist_ptr artist = Tomahawk::Artist::get( query.value( 0 ).toUInt(), query.value( 1 ).toString() );

        al << artist;
    }

    if ( al.count() )
        emit artists( al );
    emit done();
}
