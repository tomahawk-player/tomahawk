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

#include "DatabaseCommand_CollectionStats.h"

#include "DatabaseImpl.h"
#include "Source.h"
#include "utils/Logger.h"

#include "database/TomahawkSqlQuery.h"


using namespace Tomahawk;


DatabaseCommand_CollectionStats::DatabaseCommand_CollectionStats( const source_ptr& source, QObject* parent )
    : DatabaseCommand( source, parent )
{
}


void
DatabaseCommand_CollectionStats::exec( DatabaseImpl* dbi )
{
    Q_ASSERT( source()->isLocal() || source()->id() >= 1 );
    TomahawkSqlQuery query = dbi->newquery();

    QVariantMap m;
    if ( source()->isLocal() )
    {
        query.exec( "SELECT count(*), max(mtime), (SELECT guid FROM oplog WHERE source IS NULL ORDER BY id DESC LIMIT 1) "
                    "FROM file "
                    "WHERE source IS NULL" );
    }
    else
    {
        query.prepare( "SELECT count(*), max(mtime), (SELECT lastop FROM source WHERE id = ?) "
                       "FROM file "
                       "WHERE source = ?" );
        query.addBindValue( source()->id() );
        query.addBindValue( source()->id() );
        query.exec();
    }

    if ( query.next() )
    {
        m.insert( "numfiles", query.value( 0 ).toInt() );
        m.insert( "lastmodified", query.value( 1 ).toInt() );
        m.insert( "lastop", query.value( 2 ).toString() );
    }

    emit done( m );
}
