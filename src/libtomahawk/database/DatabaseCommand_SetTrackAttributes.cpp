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

#include "DatabaseCommand_SetTrackAttributes.h"
#include "TomahawkSqlQuery.h"
#include "DatabaseImpl.h"
#include "Source.h"
#include "utils/Logger.h"

using namespace Tomahawk;

DatabaseCommand_SetTrackAttributes::DatabaseCommand_SetTrackAttributes( DatabaseCommand_SetTrackAttributes::AttributeType type, QList< QPair< QID, QString > > ids, bool toDelete )
    : DatabaseCommandLoggable()
    , m_loggable( false )
    , m_delete( toDelete )
    , m_type( type )
    , m_tracks( ids )
{
}

DatabaseCommand_SetTrackAttributes::DatabaseCommand_SetTrackAttributes( DatabaseCommand_SetTrackAttributes::AttributeType type )
    : DatabaseCommandLoggable()
    , m_loggable( false )
    , m_delete( true )
    , m_type( type )
{

}


void
DatabaseCommand_SetTrackAttributes::exec( DatabaseImpl* dbi )
{
    TomahawkSqlQuery checkquery = dbi->newquery();
    TomahawkSqlQuery delquery = dbi->newquery();
    TomahawkSqlQuery insertquery = dbi->newquery();

    QString k;
    switch ( m_type )
    {
    case EchonestCatalogId:
        k = "echonestcatalogid";
        break;
    }

    if ( m_delete && m_tracks.isEmpty() )
    {
        //delete all
        TomahawkSqlQuery delAll = dbi->newquery();
        delAll.prepare( "DELETE FROM track_attributes WHERE k = ?" );
        delAll.bindValue( 0, k );
        delAll.exec();
        return;
    }

    checkquery.prepare( "SELECT id, sortname FROM track WHERE id = ?" );
    delquery.prepare( "DELETE FROM track_attributes WHERE id = ? AND k = ?" );
    insertquery.prepare( "INSERT INTO track_attributes ( id, k, v ) VALUES( ?, ?, ? )" );

    QPair< QID, QString > track;
    foreach ( track, m_tracks )
    {
        checkquery.bindValue( 0, track.first );
        if ( !checkquery.exec() )
        {
            tLog() << "No track in track table for set track attribute command...aborting:" << track.first;
            continue;
        }

        delquery.bindValue( 0, track.first );
        delquery.bindValue( 1, k );
        delquery.exec();

        if ( m_delete )
            continue; // stop at deleting, don't insert

        insertquery.bindValue( 0, track.first );
        insertquery.bindValue( 1, k );
        insertquery.bindValue( 2, track.second );
        if ( !insertquery.exec() )
            tLog() << "Failed to insert track attribute:" << k << track.first << track.second;

    }
}
