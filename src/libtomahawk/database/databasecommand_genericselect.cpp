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

#include "databasecommand_genericselect.h"

#include "databaseimpl.h"
#include "utils/logger.h"
#include <sourcelist.h>
#include <artist.h>
#include <album.h>

using namespace Tomahawk;


DatabaseCommand_GenericSelect::DatabaseCommand_GenericSelect( const QString& sqlSelect, QObject* parent )
    : DatabaseCommand( parent )
    , m_sqlSelect( sqlSelect )
{
}


void
DatabaseCommand_GenericSelect::exec( DatabaseImpl* dbi )
{
    Q_ASSERT( source()->isLocal() || source()->id() >= 1 );
    TomahawkSqlQuery query = dbi->newquery();

    query.exec( m_sqlSelect );

    QList< query_ptr > queries;


    // Expecting
    while ( query.next() )
    {
        Tomahawk::result_ptr result = Tomahawk::result_ptr( new Tomahawk::Result() );
        Tomahawk::source_ptr s;

        QString artist, track, album;
        track = query.value( 0 ).toString();
        artist = query.value( 1 ).toString();
        album = query.value( 2 ).toString();

        Tomahawk::query_ptr qry = Tomahawk::Query::get( artist, track, album, uuid(), true ); // Only auto-resolve non-local results
        queries << qry;
    }

    emit tracks( queries );
}
