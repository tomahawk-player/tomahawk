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

#include "DatabaseCommand_GenericSelect.h"

#include "DatabaseImpl.h"
#include "SourceList.h"
#include "Artist.h"
#include "Album.h"
#include "Pipeline.h"
#include "utils/Logger.h"

using namespace Tomahawk;


DatabaseCommand_GenericSelect::DatabaseCommand_GenericSelect( const QString& sqlSelect, QueryType type, int limit, QObject* parent )
    : DatabaseCommand( parent )
    , m_sqlSelect( sqlSelect )
    , m_queryType( type )
    , m_limit( limit )
    , m_raw( false )
{
}

DatabaseCommand_GenericSelect::DatabaseCommand_GenericSelect( const QString& sqlSelect, QueryType type, bool rawData, QObject* parent )
    : DatabaseCommand( parent )
    , m_sqlSelect( sqlSelect )
    , m_queryType( type )
    , m_limit( -1 )
    , m_raw( rawData )
{
}

void
DatabaseCommand_GenericSelect::exec( DatabaseImpl* dbi )
{
    TomahawkSqlQuery query = dbi->newquery();

    query.prepare( QString( "%1 %2;" ).arg( m_sqlSelect ).arg( m_limit > -1 ? QString( " LIMIT %1" ).arg( m_limit ) : QString() ) );
    query.exec();

    QList< query_ptr > queries;
    QList< artist_ptr > arts;
    QList< album_ptr > albs;

    if ( m_raw )
    {
        QList< QStringList > rawDataItems;

        while( query.next() )
        {
            QStringList rawRow;
            int count = 0;
            while ( query.value( count ).isValid() )
            {
                rawRow << query.value( count ).toString();
                ++count;
            }
            rawDataItems << rawRow;
        }
        emit rawData( rawDataItems );
        return;
    }

    // Expecting
    while ( query.next() )
    {
        query_ptr qry;
        artist_ptr artist;
        album_ptr album;

        if ( m_queryType == Track )
        {
            QString artist, track;
            track = query.value( 0 ).toString();
            artist = query.value( 1 ).toString();

            qry = Tomahawk::Query::get( artist, track, QString() );
            if ( qry.isNull() )
                continue;
        }
        else if ( m_queryType == Artist )
        {
            int artistId = query.value( 0 ).toInt();
            QString artistName = query.value( 1 ).toString();

            artist = Tomahawk::Artist::get( artistId, artistName );
        }
        else if ( m_queryType == Album )
        {
            int albumId = query.value( 0 ).toInt();
            QString albumName = query.value( 1 ).toString();
            int artistId = query.value( 2 ).toInt();
            QString artistName = query.value( 3 ).toString();

            artist = Tomahawk::Artist::get( artistId, artistName );
            album = Tomahawk::Album::get( albumId, albumName, artist );
        }

        QVariantList extraData;
        int count = 2;
        while ( query.value( count ).isValid() )
        {
            extraData << query.value( count );
            count++;
        }

        if ( m_queryType == Track )
        {
            if ( !extraData.isEmpty() )
                qry->setProperty( "data", extraData );
            queries << qry;
        }
        else if ( m_queryType == Artist )
        {
            if ( !extraData.isEmpty() )
                artist->setProperty( "data", extraData );
            arts << artist;
        }
        else if ( m_queryType == Album )
        {
            if ( !extraData.isEmpty() )
                album->setProperty( "data", extraData );
            albs << album;
        }
    }

    if ( m_queryType == Track )
        emit tracks( queries );
    else if ( m_queryType == Artist )
        emit artists( arts );
    else if ( m_queryType == Album )
        emit albums( albs );
}
