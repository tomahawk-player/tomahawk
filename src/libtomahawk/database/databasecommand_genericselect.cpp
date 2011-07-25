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
        artist = query.value( 0 ).toString();
        album = query.value( 1 ).toString();
        track = query.value( 2 ).toString();

        Tomahawk::query_ptr qry = Tomahawk::Query::get( artist, track, album, uuid(), (query.value( 7 ).toUInt() != 0) ); // Only auto-resolve non-local results
        Tomahawk::artist_ptr artistptr = Tomahawk::Artist::get( query.value( 12 ).toUInt(), artist );
        Tomahawk::album_ptr albumptr = Tomahawk::Album::get( query.value( 13 ).toUInt(), album, artistptr );

        // If it's a local track, set to the local source and url in the result and we're done. otherwise, we get resolved
        if( query.value( 7 ).toUInt() == 0 )
        {
            s = SourceList::instance()->getLocal();
            result->setUrl( query.value( 7 ).toString() );

            result->setId( query.value( 9 ).toUInt() );
            result->setArtist( artistptr );
            result->setAlbum( albumptr );
            result->setTrack( query.value( 2 ).toString() );
            result->setSize( query.value( 3 ).toUInt() );
            result->setDuration( query.value( 4 ).toUInt() );
            result->setBitrate( query.value( 5 ).toUInt() );
            result->setMimetype( query.value( 8 ).toString() );
            result->setScore( 1.0 );
            result->setCollection( s->collection() );

            TomahawkSqlQuery attrQuery = dbi->newquery();
            QVariantMap attr;

            attrQuery.prepare( "SELECT k, v FROM track_attributes WHERE id = ?" );
            attrQuery.bindValue( 0, result->dbid() );
            attrQuery.exec();
            while ( attrQuery.next() )
            {
                attr[ attrQuery.value( 0 ).toString() ] = attrQuery.value( 1 ).toString();
            }

            result->setAttributes( attr );

            qry->addResults( QList<Tomahawk::result_ptr>() << result );
            qry->setResolveFinished( true );
        }

        queries << qry;
    }

    emit tracks( queries );
}
