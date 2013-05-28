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

#include "DatabaseCommand_TrackAttributes.h"
#include "DatabaseImpl.h"
#include "Source.h"
#include "database/TomahawkSqlQuery.h"

using namespace Tomahawk;
DatabaseCommand_TrackAttributes::DatabaseCommand_TrackAttributes( DatabaseCommand_SetTrackAttributes::AttributeType type, const QList< Tomahawk::QID > ids )
    : DatabaseCommand()
    , m_type( type )
    , m_ids( ids )
{
}

DatabaseCommand_TrackAttributes::DatabaseCommand_TrackAttributes( DatabaseCommand_SetTrackAttributes::AttributeType type )
    : DatabaseCommand()
    , m_type( type )
{
}

void DatabaseCommand_TrackAttributes::exec( DatabaseImpl* lib )
{
    TomahawkSqlQuery query = lib->newquery();

    QString k;
    switch ( m_type )
    {
        case DatabaseCommand_SetTrackAttributes::EchonestCatalogId:
            k = "echonestcatalogid";
            break;
    }

    PairList results;
    if ( !m_ids.isEmpty() )
    {
        foreach ( const QID id, m_ids )
        {
            query.prepare( "SELECT v FROM track_attributes WHERE id = ? AND k = ?" );
            query.bindValue( 0, id );
            query.bindValue( 1, k );
            if ( query.exec() )
                results.append( QPair< QID, QString >( id, query.value( 0 ).toString() ) );
        }
    }
    else
    {
        query.prepare( "SELECT id, v FROM track_attributes WHERE k = ?" );
        query.bindValue( 0, k );
        query.exec();
        while ( query.next() )
        {
            results.append( QPair< QID, QString >( query.value( 0 ).toString(), query.value( 1 ).toString() ) );
        }
    }

    emit trackAttributes( results );
}
