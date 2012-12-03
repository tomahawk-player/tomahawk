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

#include "DatabaseCommand_AllArtists.h"

#include <QSqlQuery>

#include "Artist.h"
#include "DatabaseImpl.h"
#include "Source.h"
#include "utils/TomahawkUtils.h"
#include "utils/Logger.h"


DatabaseCommand_AllArtists::DatabaseCommand_AllArtists( const Tomahawk::collection_ptr& collection, QObject* parent )
    : DatabaseCommand( parent )
    , m_collection( collection )
    , m_amount( 0 )
    , m_sortOrder( DatabaseCommand_AllArtists::None )
    , m_sortDescending( false )
{
}


DatabaseCommand_AllArtists::~DatabaseCommand_AllArtists()
{
}


void
DatabaseCommand_AllArtists::exec( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();
    QString orderToken, sourceToken, filterToken, tables, joins;

    switch ( m_sortOrder )
    {
        case 0:
            break;

        case ModificationTime:
            orderToken = "file.mtime";
    }

    if ( !m_collection.isNull() )
        sourceToken = QString( "AND file.source %1" ).arg( m_collection->source()->isLocal() ? "IS NULL" : QString( "= %1" ).arg( m_collection->source()->id() ) );

    if ( !m_filter.isEmpty() )
    {
        QString filtersql;
        QStringList sl = m_filter.split( " ", QString::SkipEmptyParts );
        foreach( QString s, sl )
        {
            filtersql += QString( " AND ( artist.name LIKE '%%1%' OR album.name LIKE '%%1%' OR track.name LIKE '%%1%' )" ).arg( TomahawkSqlQuery::escape( s ) );
        }

        filterToken = QString( "AND file_join.track = track.id %1" ).arg( filtersql );
        joins = "LEFT JOIN album ON album.id = file_join.album";
        tables = "artist, track, file, file_join";
    }
    else
        tables = "artist, file, file_join";

    QString sql = QString(
            "SELECT DISTINCT artist.id, artist.name "
            "FROM %1 "
            "%2 "
            "WHERE file.id = file_join.file "
            "AND file_join.artist = artist.id "
            "%3 %4 %5 %6 %7"
            ).arg( tables )
             .arg( joins )
             .arg( sourceToken )
             .arg( filterToken )
             .arg( m_sortOrder > 0 ? QString( "ORDER BY %1" ).arg( orderToken ) : QString() )
             .arg( m_sortDescending ? "DESC" : QString() )
             .arg( m_amount > 0 ? QString( "LIMIT 0, %1" ).arg( m_amount ) : QString() );

    query.prepare( sql );
    query.exec();

    QList<Tomahawk::artist_ptr> al;
    while ( query.next() )
    {
        Tomahawk::artist_ptr artist = Tomahawk::Artist::get( query.value( 0 ).toUInt(), query.value( 1 ).toString() );
        al << artist;
    }

    emit artists( al );
    emit done();
}
